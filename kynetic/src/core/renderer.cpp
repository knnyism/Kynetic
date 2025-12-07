//
// Created by kenny on 11/5/25.
//

#include "device.hpp"
#include "engine.hpp"
#include "scene.hpp"
#include "resource_manager.hpp"

#include "rendering/descriptor.hpp"
#include "rendering/pipeline.hpp"
#include "rendering/shader.hpp"
#include "rendering/texture.hpp"
#include "rendering/mesh.hpp"

#include "renderer.hpp"

#include "vk_mem_alloc.h"

using namespace kynetic;

Renderer::Renderer()
{
    Device& device = Engine::get().device();

    init_render_target();
    init_debug_resources();

    auto general_builder = GraphicsPipelineBuilder()
                               .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                               .set_polygon_mode(VK_POLYGON_MODE_FILL)
                               .set_color_attachment_format(m_render_target.format)
                               .enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                               .set_depth_format(m_depth_render_target.format)
                               .set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                               .set_multisampling_none()
                               .disable_blending();

    m_lit_shader = Engine::get().resources().load<Shader>("assets/shaders/lit.slang");
    m_lit_pipeline = std::make_unique<Pipeline>(general_builder.set_shader(m_lit_shader).build(device));

    m_mesh_lit_shader = Engine::get().resources().load<Shader>("assets/shaders/mesh_lit.slang");
    m_mesh_lit_pipeline = std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                                         .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                         .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                                         .set_color_attachment_format(m_render_target.format)
                                                         .enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                                         .set_depth_format(m_depth_render_target.format)
                                                         .set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                                         .set_multisampling_none()
                                                         .disable_blending()
                                                         .set_shader(m_mesh_lit_shader)
                                                         .build(device));

    m_clear_shader = Engine::get().resources().load<Shader>("assets/shaders/clear.slang");
    m_clear_pipeline = std::make_unique<Pipeline>(ComputePipelineBuilder().set_shader(m_clear_shader).build(device));

    m_debug_line_shader = Engine::get().resources().load<Shader>("assets/shaders/debug_line.slang");
    m_debug_line_pipeline = std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                                           .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                           .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                                           .set_color_attachment_format(m_render_target.format)
                                                           .enable_depthtest(false, VK_COMPARE_OP_ALWAYS)
                                                           .set_depth_format(m_depth_render_target.format)
                                                           .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                                           .set_multisampling_none()
                                                           .disable_blending()
                                                           .set_shader(m_debug_line_shader)
                                                           .build(device));

    m_debug_meshlet_shader = Engine::get().resources().load<Shader>("assets/shaders/debug_meshlet.slang");
    m_debug_meshlet_pipeline = std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                                              .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                              .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                                              .set_color_attachment_format(m_render_target.format)
                                                              .enable_depthtest(false, VK_COMPARE_OP_ALWAYS)
                                                              .set_depth_format(m_depth_render_target.format)
                                                              .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                                              .set_multisampling_none()
                                                              .enable_blending_alpha()
                                                              .set_shader(m_debug_meshlet_shader)
                                                              .build(device));
}

Renderer::~Renderer()
{
    destroy_render_target();
    destroy_debug_resources();
    m_deletion_queue.flush();
}

void Renderer::init_render_target()
{
    Device& device = Engine::get().device();
    VkExtent2D device_extent = device.get_extent();

    m_render_target = device.create_image(device_extent,
                                          VK_FORMAT_R16G16B16A16_SFLOAT,
                                          VMA_MEMORY_USAGE_GPU_ONLY,
                                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                              VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          VK_IMAGE_ASPECT_COLOR_BIT);
    m_depth_render_target = device.create_image(device_extent,
                                                VK_FORMAT_D32_SFLOAT,
                                                VMA_MEMORY_USAGE_GPU_ONLY,
                                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Renderer::destroy_render_target() const
{
    Device& device = Engine::get().device();
    device.destroy_image(m_render_target);
    device.destroy_image(m_depth_render_target);
}

void Renderer::init_debug_resources()
{
    Device& device = Engine::get().device();

    m_stats_buffer = device.create_buffer(sizeof(MeshletStatsData),
                                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                          VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_stats_buffer.buffer};
        m_stats_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    m_stats_readback_buffer = device.create_buffer(sizeof(MeshletStatsData) * MAX_FRAMES_IN_FLIGHT,
                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                   VMA_MEMORY_USAGE_GPU_TO_CPU);

    for (auto& m_meshlet_debug_buffer : m_meshlet_debug_buffers)
    {
        m_meshlet_debug_buffer = device.create_buffer(
            sizeof(MeshletDebugData) * MAX_DEBUG_MESHLETS,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
    }
}

void Renderer::destroy_debug_resources()
{
    Device& device = Engine::get().device();
    device.destroy_buffer(m_stats_buffer);
    device.destroy_buffer(m_stats_readback_buffer);
    for (const auto& m_meshlet_debug_buffer : m_meshlet_debug_buffers)
    {
        device.destroy_buffer(m_meshlet_debug_buffer);
    }
}

void Renderer::render_frustum_lines()
{
    Device& device = Engine::get().device();
    Scene& scene = Engine::get().scene();
    auto& ctx = device.get_context();

    auto frustum_lines = scene.get_frustum_lines();
    if (frustum_lines.empty()) return;

    size_t buffer_size = frustum_lines.size() * sizeof(DebugLineVertex);
    AllocatedBuffer line_buffer = device.create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(line_buffer); });

    memcpy(line_buffer.info.pMappedData, frustum_lines.data(), buffer_size);

    VkDeviceAddress line_buffer_address;
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = line_buffer.buffer};
        line_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    ctx.dcb.bind_pipeline(m_debug_line_pipeline.get());

    VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_debug_line_pipeline->get_set_layout(0));
    {
        DescriptorWriter writer;
        writer.write_buffer(0, scene.get_scene_buffer().buffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(device.get(), scene_descriptor);
    }
    ctx.dcb.bind_descriptors(scene_descriptor);

    DebugPushConstants push_constants;
    push_constants.vertices = line_buffer_address;
    push_constants.vertex_count = static_cast<uint32_t>(frustum_lines.size());
    push_constants.is_line = 1;

    ctx.dcb.set_push_constants(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               sizeof(DebugPushConstants),
                               &push_constants);

    uint32_t num_line_segments = static_cast<uint32_t>(frustum_lines.size()) / 2;
    ctx.dcb.draw_auto(num_line_segments * 6, 1, 0, 0);
}

void Renderer::render_meshlet_debug()
{
    Device& device = Engine::get().device();
    Scene& scene = Engine::get().scene();
    auto& ctx = device.get_context();

    const auto& debug_settings = scene.get_debug_settings();

    if (!debug_settings.show_meshlet_spheres && !debug_settings.show_meshlet_cones) return;

    uint32_t meshlet_count = std::min(m_debug_meshlet_count, MAX_DEBUG_MESHLETS);
    if (meshlet_count == 0) return;

    uint32_t read_frame = (device.get_frame_index() + MAX_FRAMES_IN_FLIGHT - 2) % MAX_FRAMES_IN_FLIGHT;

    VkDeviceAddress debug_buffer_address;
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_meshlet_debug_buffers[read_frame].buffer};
        debug_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    ctx.dcb.bind_pipeline(m_debug_meshlet_pipeline.get());

    VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_debug_meshlet_pipeline->get_set_layout(0));
    {
        DescriptorWriter writer;
        writer.write_buffer(0, scene.get_scene_buffer().buffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.update_set(device.get(), scene_descriptor);
    }
    ctx.dcb.bind_descriptors(scene_descriptor);

    DebugMeshletPushConstants push_constants;
    push_constants.meshlet_debug_data = debug_buffer_address;
    push_constants.meshlet_count = meshlet_count;
    push_constants.show_spheres = debug_settings.show_meshlet_spheres ? 1 : 0;
    push_constants.show_cones = debug_settings.show_meshlet_cones ? 1 : 0;

    ctx.dcb.set_push_constants(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               sizeof(DebugMeshletPushConstants),
                               &push_constants);

    constexpr uint32_t SPHERE_SEGMENTS = 16;
    constexpr uint32_t SPHERE_RINGS = 12;
    constexpr uint32_t VERTICES_PER_SPHERE = SPHERE_SEGMENTS * SPHERE_RINGS * 6;
    constexpr uint32_t CONE_SEGMENTS = 16;
    constexpr uint32_t VERTICES_PER_CONE = CONE_SEGMENTS * 3 + CONE_SEGMENTS * 3;

    uint32_t total_vertices = 0;
    if (debug_settings.show_meshlet_spheres) total_vertices += meshlet_count * VERTICES_PER_SPHERE;
    if (debug_settings.show_meshlet_cones) total_vertices += meshlet_count * VERTICES_PER_CONE;

    ctx.dcb.draw_auto(total_vertices, 1, 0, 0);
}

void Renderer::readback_stats()
{
    Device& device = Engine::get().device();
    Scene& scene = Engine::get().scene();
    auto& ctx = device.get_context();

    uint32_t read_frame = (device.get_frame_index() + MAX_FRAMES_IN_FLIGHT - 2) % MAX_FRAMES_IN_FLIGHT;

    void* data;
    vmaMapMemory(device.get_allocator(), m_stats_readback_buffer.allocation, &data);

    MeshletStatsData* stats =
        reinterpret_cast<MeshletStatsData*>(static_cast<char*>(data) + read_frame * sizeof(MeshletStatsData));

    auto& debug_settings = scene.get_debug_settings();
    debug_settings.total_meshlets = stats->total_meshlets;
    debug_settings.visible_meshlets = stats->visible_meshlets;

    m_debug_meshlet_count = stats->pad0;

    vmaUnmapMemory(device.get_allocator(), m_stats_readback_buffer.allocation);

    uint32_t write_frame = device.get_frame_index();

    VkBufferCopy copy_region;
    copy_region.srcOffset = 0;
    copy_region.dstOffset = write_frame * sizeof(MeshletStatsData);
    copy_region.size = sizeof(MeshletStatsData);

    ctx.dcb.copy_buffer(m_stats_buffer.buffer, m_stats_readback_buffer.buffer, 1, &copy_region);
}

void Renderer::render_debug_visualizations()
{
    render_frustum_lines();
    render_meshlet_debug();
}

void Renderer::render()
{
    Device& device = Engine::get().device();
    ResourceManager& resources = Engine::get().resources();
    Scene& scene = Engine::get().scene();

    auto& ctx = device.get_context();
    const auto& video_out = device.get_video_out();
    const VkExtent2D device_extent = device.get_extent();

    auto& debug_settings = scene.get_debug_settings();

    if (ImGui::Begin("Render Debug"))
    {
        ImGui::SliderFloat("RenderScale", &m_render_scale, 0.3f, 1.f);
        combo_enum(m_render_channel);
        combo_enum(m_rendering_method);

        ImGui::Separator();
        ImGui::Text("Meshlet Debug");

        bool was_paused = debug_settings.pause_culling;
        if (ImGui::Checkbox("Pause Culling", &debug_settings.pause_culling))
        {
            if (debug_settings.pause_culling && !was_paused)
                scene.freeze_culling_camera();
            else if (!debug_settings.pause_culling && was_paused)
                scene.unfreeze_culling_camera();
        }

        ImGui::Checkbox("Show Frustum", &debug_settings.show_frustum);
        ImGui::Checkbox("Show Meshlet Spheres", &debug_settings.show_meshlet_spheres);
        ImGui::Checkbox("Show Meshlet Cones", &debug_settings.show_meshlet_cones);

        ImGui::Separator();
        ImGui::Text("Meshlet Statistics");
        ImGui::Text("Total Meshlets: %u", debug_settings.total_meshlets);
        ImGui::Text("Visible Meshlets: %u", debug_settings.visible_meshlets);
        if (debug_settings.total_meshlets > 0)
        {
            float cull_ratio = 1.0f - (static_cast<float>(debug_settings.visible_meshlets) /
                                       static_cast<float>(debug_settings.total_meshlets));
            ImGui::Text("Cull Ratio: %.1f%%", cull_ratio * 100.0f);
        }
    }
    ImGui::End();

    if (device_extent.width != m_last_device_extent.width || device_extent.height != m_last_device_extent.height)
    {
        m_last_device_extent = device_extent;

        destroy_render_target();
        init_render_target();
    }

    ctx.dcb.transition_image(m_render_target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    ctx.dcb.bind_pipeline(m_clear_pipeline.get());
    {
        VkDescriptorSet image_descriptor = ctx.allocator.allocate(m_clear_pipeline->get_set_layout(0));
        {
            DescriptorWriter writer;
            writer.write_image(0,
                               m_render_target.view,
                               VK_NULL_HANDLE,
                               VK_IMAGE_LAYOUT_GENERAL,
                               VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            writer.update_set(device.get(), image_descriptor);
        }
        ctx.dcb.bind_descriptors(image_descriptor);
    }
    ctx.dcb.dispatch(static_cast<uint32_t>(std::ceilf(static_cast<float>(m_render_target.extent.width) / 16.0f)),
                     static_cast<uint32_t>(std::ceilf(static_cast<float>(m_render_target.extent.height) / 16.0f)),
                     1);

    ctx.dcb.transition_image(m_render_target.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ctx.dcb.transition_image(m_depth_render_target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    const VkExtent2D draw_extent = {
        .width = static_cast<uint32_t>(static_cast<float>(m_render_target.extent.width) * m_render_scale),
        .height = static_cast<uint32_t>(static_cast<float>(m_render_target.extent.height) * m_render_scale)};

    scene.cull(m_rendering_method);

    if (m_rendering_method == RenderMode::GpuDrivenMeshlets)
    {
        MeshletStatsData zero_stats = {0, 0, 0, 0};
        AllocatedBuffer staging =
            device.create_buffer(sizeof(MeshletStatsData), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(staging); });
        memcpy(staging.info.pMappedData, &zero_stats, sizeof(MeshletStatsData));

        VkBufferCopy clear_copy;
        clear_copy.srcOffset = 0;
        clear_copy.dstOffset = 0;
        clear_copy.size = sizeof(MeshletStatsData);
        ctx.dcb.copy_buffer(staging.buffer, m_stats_buffer.buffer, 1, &clear_copy);

        VkMemoryBarrier memory_barrier{};
        memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        ctx.dcb.pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT | VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,
                                 0,
                                 1,
                                 &memory_barrier,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr);
    }

    VkRenderingAttachmentInfo color_attachment =
        vk_init::attachment_info(m_render_target.view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depth_attachment =
        vk_init::depth_attachment_info(m_depth_render_target.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo render_info = vk_init::rendering_info(draw_extent, &color_attachment, &depth_attachment);

    ctx.dcb.begin_rendering(render_info);
    {
        ctx.dcb.set_viewport(static_cast<float>(draw_extent.width), static_cast<float>(draw_extent.height));
        ctx.dcb.set_scissor(draw_extent.width, draw_extent.height);

        if (m_rendering_method == RenderMode::CpuDriven || m_rendering_method == RenderMode::GpuDriven)
        {
            ctx.dcb.bind_pipeline(m_lit_pipeline.get());

            DrawPushConstants push_constants;
            push_constants.positions = resources.m_merged_position_buffer_address;
            push_constants.vertices = resources.m_merged_vertex_buffer_address;
            push_constants.materials = resources.m_material_buffer_address;
            push_constants.instances = scene.get_instance_buffer_address();

            ctx.dcb.set_push_constants(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                       sizeof(DrawPushConstants),
                                       &push_constants);
            ctx.dcb.bind_index_buffer(resources.m_merged_index_buffer.buffer, VK_INDEX_TYPE_UINT32);

            VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_lit_pipeline->get_set_layout(0));
            {
                DescriptorWriter writer;
                writer.write_buffer(0,
                                    scene.get_scene_buffer().buffer,
                                    sizeof(SceneData),
                                    0,
                                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                writer.update_set(device.get(), scene_descriptor);
            }
            ctx.dcb.bind_descriptors(scene_descriptor);
            ctx.dcb.bind_descriptors(device.get_bindless_set(), 1);
        }
        else if (m_rendering_method == RenderMode::GpuDrivenMeshlets)
        {
            uint32_t write_frame = device.get_frame_index();

            ctx.dcb.bind_pipeline(m_mesh_lit_pipeline.get());

            VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_mesh_lit_pipeline->get_set_layout(0));
            {
                DescriptorWriter writer;
                writer.write_buffer(0,
                                    scene.get_scene_buffer().buffer,
                                    sizeof(SceneData),
                                    0,
                                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                writer.write_buffer(1, m_stats_buffer.buffer, sizeof(MeshletStatsData), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
                writer.write_buffer(2,
                                    m_meshlet_debug_buffers[write_frame].buffer,
                                    sizeof(MeshletDebugData) * MAX_DEBUG_MESHLETS,
                                    0,
                                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
                writer.update_set(device.get(), scene_descriptor);
            }
            ctx.dcb.bind_descriptors(scene_descriptor);
            ctx.dcb.bind_descriptors(device.get_bindless_set(), 1);

            MeshDrawPushConstants push_constants;
            push_constants.draws = scene.get_mesh_draw_data_buffer_address();
            push_constants.materials = resources.m_material_buffer_address;
            push_constants.instances = scene.get_instance_buffer_address();

            ctx.dcb.set_push_constants(
                VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT,
                sizeof(MeshDrawPushConstants),
                &push_constants);

            ctx.dcb.multi_draw_mesh_tasks_indirect(scene.get_mesh_indirect_buffer().buffer,
                                                   scene.get_mesh_draw_count(),
                                                   sizeof(VkDrawMeshTasksIndirectCommandEXT),
                                                   0);
        }

        scene.draw(m_rendering_method);

        if (m_rendering_method == RenderMode::GpuDrivenMeshlets)
        {
            render_debug_visualizations();
        }
    }
    ctx.dcb.end_rendering();

    if (m_rendering_method == RenderMode::GpuDrivenMeshlets)
    {
        VkMemoryBarrier readback_barrier{};
        readback_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        readback_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        readback_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        ctx.dcb.pipeline_barrier(VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT | VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 1,
                                 &readback_barrier,
                                 0,
                                 nullptr,
                                 0,
                                 nullptr);

        readback_stats();
    }

    ctx.dcb.transition_image(m_render_target.image,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    ctx.dcb.transition_image(video_out, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ctx.dcb.copy_image_to_image(m_render_target.image, video_out, draw_extent, device_extent);
}