//
// Created by kenny on 11/19/25.
//

#include "core/engine.hpp"
#include "core/device.hpp"

#include "core/components.hpp"
#include "rendering/material.hpp"
#include "rendering/mesh.hpp"
#include "rendering/model.hpp"

#include "scene.hpp"

using namespace kynetic;

Scene::Scene()
{
    m_root = m_scene.entity().add<TransformComponent>();

    m_scene.observer<TransformComponent>("TransformDirty")
        .event(flecs::OnSet)
        .each([](TransformComponent& transform) { transform.is_dirty = true; });
}

void Scene::update()
{
    m_scene.query_builder<TransformComponent>().each(
        [](const flecs::entity entity, const TransformComponent& transform)
        {
            if (transform.is_dirty)
                entity.children<TransformComponent>(
                    [](const flecs::entity child)
                    {
                        if (auto* child_transform = child.try_get_mut<TransformComponent>()) child_transform->is_dirty = true;
                    });
        });

    m_scene.query_builder<TransformComponent, TransformComponent>().term_at(1).parent().cascade().build().each(
        [](TransformComponent& transform, const TransformComponent& transform_parent)
        {
            if (transform.is_dirty)
            {
                transform.is_dirty = false;
                transform.transform = transform_parent.transform *
                                      make_transform_matrix(transform.translation, transform.rotation, transform.scale);
            }
        });

    const Device& device = Engine::get().device();
    const float aspect = static_cast<float>(device.get_extent().width) / static_cast<float>(device.get_extent().height);

    m_scene.query_builder<CameraComponent, TransformComponent, MainCameraTag>().build().each(
        [&](const CameraComponent& camera, const TransformComponent& transform, const MainCameraTag&)
        {
            m_view = glm::inverse(transform.transform);
            m_projection =
                glm::perspective(camera.fovy * glm::radians(0.5f), aspect * camera.aspect, camera.far_plane, camera.near_plane);
            m_projection_debug =
                glm::perspective(camera.fovy * 2.f, aspect * camera.aspect, camera.far_plane, camera.near_plane);
            m_projection[1][1] *= -1;
            m_projection_debug[1][1] *= -1;
        });

    constexpr bool NO_INSTANCING = false;

    uint32_t instance_count = 0;
    if (NO_INSTANCING)
    {
        m_draw_commands.clear();
        m_instances.clear();

        m_scene.query_builder<TransformComponent, MeshComponent>().build().each(
            [&](const TransformComponent& transform, const MeshComponent& mesh_component)
            {
                for (const auto& mesh : mesh_component.meshes)
                {
                    VkDrawIndexedIndirectCommand& batch = m_draw_commands.emplace_back();
                    batch.firstIndex = mesh->get_index_offset();
                    batch.indexCount = mesh->get_index_count();
                    batch.firstInstance = instance_count++;
                    batch.instanceCount = 1;
                    batch.vertexOffset = static_cast<int32_t>(mesh->get_vertex_offset());

                    glm::vec3 world_center = glm::vec3(transform.transform * glm::vec4(mesh->get_centroid(), 1.0f));

                    float max_scale = glm::max(glm::length(glm::vec3(transform.transform[0])),
                                               glm::max(glm::length(glm::vec3(transform.transform[1])),
                                                        glm::length(glm::vec3(transform.transform[2]))));

                    m_instances.push_back(
                        InstanceData{.model = transform.transform,
                                     .model_inv = glm::transpose(glm::inverse(glm::mat3(transform.transform))),
                                     .position = glm::vec4(world_center, mesh->get_radius() * max_scale),
                                     .material_index = mesh->get_material()->get_handle()});
                }
            });
    }
    else
    {
        std::unordered_map<Mesh*, std::vector<InstanceData>> instances_by_mesh;

        m_scene.query_builder<TransformComponent, MeshComponent>().build().each(
            [&](const TransformComponent& transform, const MeshComponent& mesh_component)
            {
                for (const auto& mesh : mesh_component.meshes)
                {
                    glm::vec3 world_center = glm::vec3(transform.transform * glm::vec4(mesh->get_centroid(), 1.0f));

                    float max_scale = glm::max(glm::length(glm::vec3(transform.transform[0])),
                                               glm::max(glm::length(glm::vec3(transform.transform[1])),
                                                        glm::length(glm::vec3(transform.transform[2]))));

                    instances_by_mesh[mesh.get()].push_back(
                        InstanceData{.model = transform.transform,
                                     .model_inv = glm::transpose(glm::inverse(glm::mat3(transform.transform))),
                                     .position = glm::vec4(world_center, mesh->get_radius() * max_scale),
                                     .material_index = mesh->get_material()->get_handle()});
                    instance_count++;
                }
            });

        const bool should_rebuild_draw_commands = m_instance_count != instance_count;
        m_instance_count = instance_count;

        m_instances.resize(instance_count);

        uint32_t instance_index = 0;
        if (should_rebuild_draw_commands)
        {
            m_draw_commands.clear();
            for (auto [mesh, instance_data] : instances_by_mesh)
            {
                VkDrawIndexedIndirectCommand& batch = m_draw_commands.emplace_back();
                batch.firstIndex = mesh->get_index_offset();
                batch.indexCount = mesh->get_index_count();
                batch.firstInstance = instance_index;
                batch.instanceCount = static_cast<uint32_t>(instance_data.size());
                batch.vertexOffset = static_cast<int32_t>(mesh->get_vertex_offset());

                for (auto data : instance_data)
                {
                    m_instances[instance_index] = data;
                    instance_index++;
                }
            }
        }
        else
        {
            for (auto& [mesh, instance_data] : instances_by_mesh)
            {
                for (auto& data : instance_data)
                {
                    m_instances[instance_index] = data;
                    instance_index++;
                }
            }
        }
    }
}

bool Scene::update_instance_data_buffer()
{
    Device& device = Engine::get().device();

    m_instance_data_size = m_instances.size() * sizeof(InstanceData);

    uint32_t frame_index = device.get_frame_index();
    Context& ctx = device.get_context();

    auto& instance_data_buffer = m_instance_data_buffers[frame_index];

    instance_data_buffer = device.create_buffer(
        m_instance_data_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(instance_data_buffer); });

    const AllocatedBuffer staging =
        device.create_buffer(m_instance_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(staging); });

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);
    memcpy(data, m_instances.data(), m_instance_data_size);
    vmaUnmapMemory(device.get_allocator(), staging.allocation);

    VkBufferCopy copy{};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = m_instance_data_size;

    ctx.dcb.copy_buffer(staging.buffer, instance_data_buffer.buffer, 1, &copy);

    return false;
}

void Scene::update_indirect_commmand_buffer()
{
    Device& device = Engine::get().device();

    uint32_t frame_index = device.get_frame_index();
    Context& ctx = device.get_context();

    auto& indirect_command_buffer = m_indirect_command_buffer[frame_index];

    size_t indirect_buffer_size = sizeof(VkDrawIndexedIndirectCommand) * m_draw_commands.size();

    indirect_command_buffer = device.create_buffer(indirect_buffer_size,
                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                       VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                   VMA_MEMORY_USAGE_GPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(indirect_command_buffer); });

    AllocatedBuffer staging =
        device.create_buffer(indirect_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(staging); });

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);
    memcpy(data, m_draw_commands.data(), indirect_buffer_size);
    vmaUnmapMemory(device.get_allocator(), staging.allocation);

    VkBufferCopy copy{};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = indirect_buffer_size;

    ctx.dcb.copy_buffer(staging.buffer, indirect_command_buffer.buffer, 1, &copy);
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
        const flecs::entity child_entity = m_scene.entity().child_of(parent).add<TransformComponent>();

        if (!node.meshes.empty())
        {
            child_entity.add<MeshComponent>();
            child_entity.set<MeshComponent>({node.meshes});
        }

        child_entity.set<TransformComponent>({.translation = node.translation, .rotation = node.rotation, .scale = node.scale});

        for (const auto& child : node.children) traverse_nodes(child, child_entity);
    };

    traverse_nodes(model->get_root_node(), root_entity);

    return root_entity;
}

VkDeviceAddress Scene::get_instance_data_buffer_address() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    const VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                        .buffer = m_instance_data_buffers[frame_index].buffer};
    return vkGetBufferDeviceAddress(device.get(), &device_address_info);
}

VkBuffer Scene::get_instance_data_buffer() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    return m_instance_data_buffers[frame_index].buffer;
}

VkBuffer Scene::get_indirect_commmand_buffer() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    return m_indirect_command_buffer[frame_index].buffer;
}

VkDeviceAddress Scene::get_indirect_commmand_buffer_address() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    const VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                        .buffer = m_indirect_command_buffer[frame_index].buffer};
    return vkGetBufferDeviceAddress(device.get(), &device_address_info);
}
