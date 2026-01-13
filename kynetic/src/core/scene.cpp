//
// Created by kenny on 11/19/25.
//

#include "engine.hpp"
#include "device.hpp"
#include "resource_manager.hpp"
#include "components.hpp"

#include "rendering/shader.hpp"
#include "rendering/pipeline.hpp"
#include "rendering/material.hpp"
#include "rendering/mesh.hpp"
#include "rendering/model.hpp"

#include "scene.hpp"

using namespace kynetic;

static void get_frustum_planes(glm::mat4x4& view_projection, glm::vec4* out)
{
    for (auto i = 0; i < 3; ++i)
    {
        for (size_t j = 0; j < 2; ++j)
        {
            const float sign = j ? 1.f : -1.f;
            for (auto k = 0; k < 4; ++k) out[2 * i + j][k] = view_projection[k][3] + sign * view_projection[k][i];
        }
    }

    // Appendix A.2 – Normalizing the Plane Equation
    for (int plane = 0; plane < 6; ++plane) out[plane] /= glm::length(glm::vec3(out[plane]));
}

static bool is_visible(glm::vec4* planes, glm::vec3 origin, float radius)
{
    // We ignore the near and far planes as we're far less likely to cull
    // objects based on those planes. Especially with games that have big
    // values for the far plane.

    // For example, an object moving left relative to the camera is more
    // likely to leave the screen faster than an object moving away from
    // the camera.

    std::array<int, 4> V{0, 1, 4, 5};

    // Return true if all planes have the sphere on the positive side
    return std::ranges::all_of(V,
                               [planes, origin, radius](size_t i)
                               {
                                   const auto& plane = planes[i];
                                   return dot(origin, glm::vec3(plane)) + plane.w + radius >= 0;
                               });
}

Scene::Scene()
{
    m_root = m_scene.entity().add<TransformComponent>();

    m_scene.observer<TransformComponent>("TransformDirty")
        .event(flecs::OnSet)
        .each([](TransformComponent& transform) { transform.is_dirty = true; });

    m_transform_dirty_query = m_scene.query_builder<TransformComponent>().build();

    m_transform_hierarchy_query =
        m_scene.query_builder<TransformComponent, TransformComponent>().term_at(1).parent().cascade().build();

    m_camera_query = m_scene.query_builder<CameraComponent, TransformComponent, MainCameraTag>().build();

    m_mesh_query =
        m_scene.query_builder<TransformComponent, MeshComponent>()
            .term_at(1)
            .in()
            .order_by<MeshComponent>([](flecs::entity_t, const MeshComponent* m1, flecs::entity_t, const MeshComponent* m2)
                                     { return (m1->mesh.get() > m2->mesh.get()) - (m1->mesh.get() < m2->mesh.get()); })
            .build();

    m_mesh_query_unordered = m_scene.query_builder<TransformComponent, MeshComponent>().build();

    m_cull_shader = Engine::get().resources().load<Shader>("assets/shaders/cull.slang");
    m_cull_pipeline =
        std::make_unique<Pipeline>(ComputePipelineBuilder().set_shader(m_cull_shader).build(Engine::get().device()));

    m_mesh_cull_shader = Engine::get().resources().load<Shader>("assets/shaders/mesh_cull.slang");
    m_mesh_cull_pipeline =
        std::make_unique<Pipeline>(ComputePipelineBuilder().set_shader(m_mesh_cull_shader).build(Engine::get().device()));
}

Scene::~Scene() = default;

void Scene::gpu_cull() const
{
    if (get_instance_output_buffer().buffer == VK_NULL_HANDLE) return;

    Device& device = Engine::get().device();
    Context& ctx = device.get_context();

    ctx.dcb.bind_pipeline(m_cull_pipeline.get());

    VkDescriptorSet cull_descriptor = ctx.allocator.allocate(m_cull_pipeline->get_set_layout(0));
    {
        DescriptorWriter writer;
        writer.write_buffer(0, get_scene_buffer().buffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.write_buffer(1, get_instance_buffer().buffer, VK_WHOLE_SIZE, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.write_buffer(2, get_instance_output_buffer().buffer, VK_WHOLE_SIZE, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.update_set(device.get(), cull_descriptor);
    }
    ctx.dcb.bind_descriptors(cull_descriptor);

    FrustumCullPushConstants push_constants;
    push_constants.draw_count = static_cast<uint32_t>(m_draws.size());
    push_constants.instance_count = static_cast<uint32_t>(m_instances.size());
    push_constants.draw_commands = m_draw_buffer_address;

    ctx.dcb.set_push_constants(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(FrustumCullPushConstants), &push_constants);

    const uint32_t dispatch_x = push_constants.instance_count > 0 ? 1 + (push_constants.instance_count - 1) / 64 : 1;
    ctx.dcb.dispatch(dispatch_x, 1, 1);

    ctx.dcb.pipeline_barrier(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                             VK_ACCESS_2_SHADER_WRITE_BIT,
                             VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
                             VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
}

void Scene::gpu_cull_mesh() const
{
    Device& device = Engine::get().device();
    Context& ctx = device.get_context();

    ctx.dcb.bind_pipeline(m_mesh_cull_pipeline.get());

    VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_mesh_cull_pipeline->get_set_layout(0));
    {
        DescriptorWriter writer;
        writer.write_buffer(0, get_scene_buffer().buffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.write_buffer(1, get_instance_buffer().buffer, VK_WHOLE_SIZE, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        writer.update_set(device.get(), scene_descriptor);
    }
    ctx.dcb.bind_descriptors(scene_descriptor);

    size_t draw_count = m_mesh_indirect_commands.size();

    FrustumCullPushConstants push_constants;
    push_constants.draw_count = static_cast<uint32_t>(draw_count);
    push_constants.draw_commands = m_mesh_indirect_buffer_address;
    ctx.dcb.set_push_constants(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(FrustumCullPushConstants), &push_constants);

    const uint32_t dispatch_x = draw_count > 0 ? 1 + (draw_count - 1) / 64 : 1;
    ctx.dcb.dispatch(dispatch_x, 1, 1);

    ctx.dcb.pipeline_barrier(VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                             VK_ACCESS_2_SHADER_WRITE_BIT,
                             VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
                             VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
}

void Scene::freeze_culling_camera()
{
    m_debug_settings.pause_culling = true;
    m_debug_settings.frozen_view = m_view;
    m_debug_settings.frozen_projection = m_projection;
    m_debug_settings.frozen_vp = m_projection * m_view;
    m_debug_settings.frozen_camera_position = glm::vec3(glm::inverse(m_view)[3]);
    m_debug_settings.frozen_previous_vp = m_previous_vp;
}

void Scene::unfreeze_culling_camera() { m_debug_settings.pause_culling = false; }

std::vector<glm::vec3> Scene::get_frustum_corners(const glm::mat4& vp) const
{
    glm::mat4 inv_vp = glm::inverse(vp);

    std::vector<glm::vec3> corners;
    corners.reserve(8);

    const float ndc_coords[8][3] =
        {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}};

    for (auto ndc_coord : ndc_coords)
    {
        glm::vec4 corner = inv_vp * glm::vec4(ndc_coord[0], ndc_coord[1], ndc_coord[2], 1.0f);
        corners.push_back(glm::vec3(corner) / corner.w);
    }

    return corners;
}

std::vector<DebugLineVertex> Scene::get_frustum_lines() const
{
    std::vector<DebugLineVertex> vertices;

    if (!m_debug_settings.pause_culling || !m_debug_settings.show_frustum) return vertices;

    auto corners = get_frustum_corners(m_debug_settings.frozen_vp);

    glm::vec4 color{1.0f, 1.0f, 0.0f, 1.0f};

    auto add_line = [&](const glm::vec3& a, const glm::vec3& b)
    {
        vertices.push_back({a, 0.0f, color});
        vertices.push_back({b, 0.0f, color});
    };

    // Near
    add_line(corners[0], corners[1]);
    add_line(corners[1], corners[2]);
    add_line(corners[2], corners[3]);
    add_line(corners[3], corners[0]);

    // Far
    add_line(corners[4], corners[5]);
    add_line(corners[5], corners[6]);
    add_line(corners[6], corners[7]);
    add_line(corners[7], corners[4]);

    // The rest lol
    add_line(corners[0], corners[4]);
    add_line(corners[1], corners[5]);
    add_line(corners[2], corners[6]);
    add_line(corners[3], corners[7]);

    return vertices;
}

void Scene::update()
{
    Device& device = Engine::get().device();

    m_transform_dirty_query.each(
        [](const flecs::entity entity, const TransformComponent& transform)
        {
            if (transform.is_dirty)
                entity.children<TransformComponent>(
                    [](const flecs::entity child)
                    {
                        if (auto* child_transform = child.try_get_mut<TransformComponent>()) child_transform->is_dirty = true;
                    });
        });

    m_transform_hierarchy_query.each(
        [](TransformComponent& transform, const TransformComponent& transform_parent)
        {
            if (transform.is_dirty)
            {
                transform.is_dirty = false;
                transform.transform = transform_parent.transform *
                                      make_transform_matrix(transform.translation, transform.rotation, transform.scale);
            }
        });

    const float aspect = static_cast<float>(device.get_extent().width) / static_cast<float>(device.get_extent().height);

    m_camera_query.each(
        [&](const CameraComponent& camera, const TransformComponent& transform, const MainCameraTag&)
        {
            m_view = glm::inverse(transform.transform);
            m_projection = glm::perspective(camera.fovy, aspect * camera.aspect, camera.far_plane, camera.near_plane);
            m_projection[1][1] *= -1;
            m_camera_fovy = camera.fovy;
        });

    glm::mat4 vp = m_projection * m_view;

    m_scene_data = SceneData{
        .view = m_view,
        .view_inv = glm::inverse(glm::transpose(m_view)),

        .proj = m_projection,
        .vp = vp,
        .previous_vp = m_debug_settings.pause_culling ? m_debug_settings.frozen_previous_vp : m_previous_vp,

        .debug_view = m_debug_settings.frozen_view,
        .debug_view_inv = glm::inverse(glm::transpose(m_debug_settings.frozen_view)),
        .debug_proj = m_debug_settings.frozen_projection,
        .debug_vp = m_debug_settings.frozen_vp,

        .ambient_color = glm::vec4(0.1f, 0.1f, 0.1f, 0.f),
        .sun_direction = glm::vec4(glm::normalize(glm::vec3(-1.f, -1.f, -1.f)), 0.f),
        .sun_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.f),

        .use_debug_culling = m_debug_settings.pause_culling ? 1u : 0u,

        .z_near = 0.1f,
        .projection_00 = m_projection[0][0],
        .projection_11 = m_projection[1][1],
    };

    get_frustum_planes(vp, m_scene_data.frustum);
    get_frustum_planes(m_debug_settings.frozen_vp, m_scene_data.debug_frustum);

    if (!m_debug_settings.pause_culling) m_previous_vp = vp;

    m_draws.clear();
    m_mesh_draw_data.clear();
    m_mesh_indirect_commands.clear();
    m_instances.clear();

    auto& cull_planes = m_debug_settings.pause_culling ? m_scene_data.debug_frustum : m_scene_data.frustum;

    Mesh* last_mesh = nullptr;

    if (m_debug_settings.render_mode == RenderMode::CpuDriven)
    {
        if (m_debug_settings.enable_frustum_culling)
        {
            m_mesh_query.each(
                [&](const TransformComponent& t, const MeshComponent& m)
                {
                    glm::vec3 world_center = glm::vec3(t.transform * glm::vec4(m.mesh->get_centroid(), 1.0f));

                    float max_scale =
                        glm::max(glm::length(glm::vec3(t.transform[0])),
                                 glm::max(glm::length(glm::vec3(t.transform[1])), glm::length(glm::vec3(t.transform[2]))));

                    if (is_visible(cull_planes, world_center, m.mesh->get_radius() * max_scale))
                    {
                        InstanceData& instance = m_instances.emplace_back();
                        instance.model = t.transform;
                        instance.model_inv = glm::transpose(glm::inverse(glm::mat3(t.transform)));
                        instance.material_index = m.mesh->get_material()->get_handle();

                        if (last_mesh == m.mesh.get())
                        {
                            m_draws.back().instanceCount++;
                        }
                        else
                        {
                            VkDrawIndexedIndirectCommand& draw = m_draws.emplace_back();
                            draw.firstIndex = m.mesh->get_index_offset();
                            draw.indexCount = m.mesh->get_index_count();
                            draw.firstInstance = static_cast<uint32_t>(m_instances.size() - 1);
                            draw.instanceCount = 1;
                            draw.vertexOffset = static_cast<int32_t>(m.mesh->get_vertex_offset());

                            last_mesh = m.mesh.get();
                        }
                    }
                });
        }
        else
        {
            m_mesh_query.each(
                [&](const TransformComponent& t, const MeshComponent& m)
                {
                    InstanceData& instance = m_instances.emplace_back();
                    instance.model = t.transform;
                    instance.model_inv = glm::transpose(glm::inverse(glm::mat3(t.transform)));
                    instance.position = glm::vec4(m.mesh->get_centroid(), m.mesh->get_radius());
                    instance.material_index = m.mesh->get_material()->get_handle();

                    if (last_mesh == m.mesh.get())
                    {
                        m_draws.back().instanceCount++;
                    }
                    else
                    {
                        VkDrawIndexedIndirectCommand& draw = m_draws.emplace_back();
                        draw.firstIndex = m.mesh->get_index_offset();
                        draw.indexCount = m.mesh->get_index_count();
                        draw.firstInstance = static_cast<uint32_t>(m_instances.size() - 1);
                        draw.instanceCount = 1;
                        draw.vertexOffset = static_cast<int32_t>(m.mesh->get_vertex_offset());

                        last_mesh = m.mesh.get();
                    }
                });
        }
    }
    else if (m_debug_settings.render_mode == RenderMode::Meshlets)
    {
        m_mesh_query_unordered.each(
            [&](const TransformComponent& t, const MeshComponent& m)
            {
                uint32_t instance_index = static_cast<uint32_t>(m_instances.size());

                InstanceData& instance = m_instances.emplace_back();
                instance.model = t.transform;
                instance.model_inv = glm::transpose(glm::inverse(glm::mat3(t.transform)));
                instance.position = glm::vec4(m.mesh->get_centroid(), m.mesh->get_radius());
                instance.material_index = m.mesh->get_material()->get_handle();

                MeshDrawData& draw_data = m_mesh_draw_data.emplace_back();
                draw_data.positions = m.mesh->get_position_buffer_address();
                draw_data.vertices = m.mesh->get_vertex_buffer_address();
                draw_data.meshlets = m.mesh->get_meshlet_buffer_address();
                draw_data.meshlet_vertices = m.mesh->get_meshlet_vertices_buffer_address();
                draw_data.meshlet_triangles = m.mesh->get_meshlet_triangles_buffer_address();
                draw_data.lod_groups = m.mesh->get_lod_groups_buffer_address();
                draw_data.instance_index = instance_index;
                draw_data.meshlet_count = static_cast<uint32_t>(m.mesh->get_meshlet_count());
                draw_data.lod_group_count = static_cast<uint32_t>(m.mesh->get_lod_group_count());

                VkDrawMeshTasksIndirectCommandEXT& indirect_cmd = m_mesh_indirect_commands.emplace_back();
                indirect_cmd.groupCountX =
                    static_cast<uint32_t>(std::ceilf(static_cast<float>(m.mesh->get_meshlet_count()) / 32.f));
                indirect_cmd.groupCountY = 1;
                indirect_cmd.groupCountZ = 1;
            });
    }
    else
    {
        uint32_t current_draw_id = 0;
        m_mesh_query.each(
            [&](const TransformComponent& t, const MeshComponent& m)
            {
                InstanceData& instance = m_instances.emplace_back();
                instance.model = t.transform;
                instance.model_inv = glm::transpose(glm::inverse(glm::mat3(t.transform)));
                instance.position = glm::vec4(m.mesh->get_centroid(), m.mesh->get_radius());
                instance.material_index = m.mesh->get_material()->get_handle();

                if (last_mesh != m.mesh.get())
                {
                    if (!m_draws.empty()) current_draw_id++;

                    VkDrawIndexedIndirectCommand& draw = m_draws.emplace_back();
                    draw.firstIndex = m.mesh->get_index_offset();
                    draw.indexCount = m.mesh->get_index_count();
                    draw.firstInstance = static_cast<uint32_t>(m_instances.size() - 1);
                    draw.instanceCount = 0;
                    draw.vertexOffset = static_cast<int32_t>(m.mesh->get_vertex_offset());

                    last_mesh = m.mesh.get();
                }

                instance.draw_id = current_draw_id;
            });
    }

    update_buffers();
}

void Scene::update_buffers()
{
    Device& device = Engine::get().device();
    Context& ctx = device.get_context();

    if (m_instances.empty()) return;

    size_t instance_buffer_size = m_instances.size() * sizeof(InstanceData);
    size_t draw_size = m_draws.size() * sizeof(VkDrawIndexedIndirectCommand);
    size_t mesh_draw_data_size = m_mesh_draw_data.size() * sizeof(MeshDrawData);
    size_t mesh_indirect_size = m_mesh_indirect_commands.size() * sizeof(VkDrawMeshTasksIndirectCommandEXT);

    uint32_t frame_index = device.get_frame_index();

    auto& instances_buffer = m_instances_buffers[frame_index];
    auto& instances_output_buffer = m_instances_output_buffers[frame_index];

    auto& draw_buffer = m_draw_buffers[frame_index];
    auto& mesh_draw_data_buffer = m_mesh_draw_data_buffers[frame_index];
    auto& mesh_indirect_buffer = m_mesh_indirect_buffers[frame_index];
    auto& scene_buffer = m_scene_buffers[frame_index];

    instances_buffer = device.create_buffer(
        instance_buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(instances_buffer); });

    {
        const VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                            .buffer = instances_buffer.buffer};
        m_instances_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    instances_output_buffer =
        device.create_buffer(instance_buffer_size,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                             VMA_MEMORY_USAGE_GPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(instances_output_buffer); });

    const VkBufferDeviceAddressInfo addr_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                              .buffer = instances_output_buffer.buffer};
    m_instances_output_buffer_address = vkGetBufferDeviceAddress(device.get(), &addr_info);

    if (draw_size > 0)
    {
        draw_buffer = device.create_buffer(draw_size,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY);
        ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(draw_buffer); });

        {
            const VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                                .buffer = draw_buffer.buffer};
            m_draw_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
        }
    }
    if (mesh_draw_data_size > 0)
    {
        mesh_draw_data_buffer = device.create_buffer(
            mesh_draw_data_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(mesh_draw_data_buffer); });

        {
            const VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                                .buffer = mesh_draw_data_buffer.buffer};
            m_mesh_draw_data_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
        }
    }

    if (mesh_indirect_size > 0)
    {
        mesh_indirect_buffer =
            device.create_buffer(mesh_indirect_size,
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                 VMA_MEMORY_USAGE_GPU_ONLY);
        ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(mesh_indirect_buffer); });

        {
            const VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                                .buffer = mesh_indirect_buffer.buffer};
            m_mesh_indirect_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
        }
    }

    scene_buffer = device.create_buffer(sizeof(SceneData),
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                        VMA_MEMORY_USAGE_GPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(scene_buffer); });

    const size_t total_staging_size =
        instance_buffer_size + draw_size + mesh_draw_data_size + mesh_indirect_size + sizeof(SceneData);

    const AllocatedBuffer staging =
        device.create_buffer(total_staging_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(staging); });

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);

    size_t offset = 0;
    memcpy(static_cast<char*>(data) + offset, m_instances.data(), instance_buffer_size);
    offset += instance_buffer_size;

    memcpy(static_cast<char*>(data) + offset, m_draws.data(), draw_size);
    offset += draw_size;

    memcpy(static_cast<char*>(data) + offset, m_mesh_draw_data.data(), mesh_draw_data_size);
    offset += mesh_draw_data_size;

    memcpy(static_cast<char*>(data) + offset, m_mesh_indirect_commands.data(), mesh_indirect_size);
    offset += mesh_indirect_size;

    memcpy(static_cast<char*>(data) + offset, &m_scene_data, sizeof(SceneData));

    vmaUnmapMemory(device.get_allocator(), staging.allocation);

    size_t src_offset = 0;

    VkBufferCopy instance_copy;
    instance_copy.dstOffset = 0;
    instance_copy.srcOffset = src_offset;
    instance_copy.size = instance_buffer_size;
    ctx.dcb.copy_buffer(staging.buffer, instances_buffer.buffer, 1, &instance_copy);
    src_offset += instance_buffer_size;

    if (draw_size > 0)
    {
        VkBufferCopy draw_copy;
        draw_copy.dstOffset = 0;
        draw_copy.srcOffset = src_offset;
        draw_copy.size = draw_size;
        ctx.dcb.copy_buffer(staging.buffer, draw_buffer.buffer, 1, &draw_copy);
        src_offset += draw_size;
    }

    if (mesh_draw_data_size > 0)
    {
        VkBufferCopy mesh_draw_data_copy;
        mesh_draw_data_copy.dstOffset = 0;
        mesh_draw_data_copy.srcOffset = src_offset;
        mesh_draw_data_copy.size = mesh_draw_data_size;
        ctx.dcb.copy_buffer(staging.buffer, mesh_draw_data_buffer.buffer, 1, &mesh_draw_data_copy);
        src_offset += mesh_draw_data_size;
    }

    if (mesh_indirect_size > 0)
    {
        VkBufferCopy mesh_indirect_copy;
        mesh_indirect_copy.dstOffset = 0;
        mesh_indirect_copy.srcOffset = src_offset;
        mesh_indirect_copy.size = mesh_indirect_size;
        ctx.dcb.copy_buffer(staging.buffer, mesh_indirect_buffer.buffer, 1, &mesh_indirect_copy);
        src_offset += mesh_indirect_size;
    }

    VkBufferCopy scene_data_copy;
    scene_data_copy.dstOffset = 0;
    scene_data_copy.srcOffset = src_offset;
    scene_data_copy.size = sizeof(SceneData);
    ctx.dcb.copy_buffer(staging.buffer, scene_buffer.buffer, 1, &scene_data_copy);
}

flecs::entity Scene::add_camera(bool is_main_camera) const
{
    const flecs::entity camera_entity =
        m_scene.entity()
            .add<TransformComponent>()
            .set<CameraComponent>({.aspect = 1.f, .near_plane = 0.1f, .far_plane = 1000.f, .fovy = 70.f})
            .child_of(m_root);

    if (is_main_camera) camera_entity.add<MainCameraTag>();

    return camera_entity;
}

flecs::entity Scene::add_model(const std::shared_ptr<Model>& model) const
{
    const flecs::entity root_entity = m_scene.entity().child_of(m_root);

    std::function<void(const Model::Node&, flecs::entity)> traverse_nodes =
        [&](const Model::Node& node, const flecs::entity parent)
    {
        const flecs::entity child_entity = m_scene.entity().child_of(parent).set<TransformComponent>(
            {.translation = node.translation, .rotation = node.rotation, .scale = node.scale});

        for (const auto& mesh : node.meshes)
            m_scene.entity().child_of(child_entity).add<TransformComponent>().set<MeshComponent>({mesh});

        for (const auto& child : node.children) traverse_nodes(child, child_entity);
    };

    traverse_nodes(model->get_root_node(), root_entity);

    return root_entity;
}

AllocatedBuffer Scene::get_instance_buffer() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    return m_instances_buffers[frame_index];
}

AllocatedBuffer Scene::get_instance_output_buffer() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();
    return m_instances_output_buffers[frame_index];
}

AllocatedBuffer Scene::get_draw_buffer() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    return m_draw_buffers[frame_index];
}

AllocatedBuffer Scene::get_scene_buffer() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    return m_scene_buffers[frame_index];
}

AllocatedBuffer Scene::get_mesh_indirect_buffer() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();
    return m_mesh_indirect_buffers[frame_index];
}

void Scene::cull() const
{
    switch (m_debug_settings.render_mode)
    {
        case RenderMode::CpuDriven:
            break;  // We perform the culling when gathering instances
        case RenderMode::GpuDriven:
        {
            if (m_debug_settings.enable_frustum_culling) gpu_cull();
        }
        break;
        case RenderMode::Meshlets:
        {
            if (m_debug_settings.enable_frustum_culling) gpu_cull_mesh();
        }
        break;
        default:
            break;
    }
}

void Scene::draw() const
{
    Device& device = Engine::get().device();
    Context& ctx = device.get_context();

    switch (m_debug_settings.render_mode)
    {
        case RenderMode::CpuDriven:
        {
            for (const auto& draw : get_draws())
                ctx.dcb.draw(draw.indexCount, draw.instanceCount, draw.firstIndex, draw.vertexOffset, draw.firstInstance);
        }
        break;
        case RenderMode::GpuDriven:
        {
            ctx.dcb.multi_draw_indirect(get_draw_buffer().buffer, get_draw_count(), sizeof(VkDrawIndexedIndirectCommand));
        }
        break;
        case RenderMode::Meshlets:
        {
            ctx.dcb.multi_draw_mesh_tasks_indirect(get_mesh_indirect_buffer().buffer,
                                                   get_mesh_draw_count(),
                                                   sizeof(VkDrawMeshTasksIndirectCommandEXT),
                                                   0);
        }
        break;
        default:
            break;
    }
}