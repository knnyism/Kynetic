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

    m_depth_pyramid_pipeline =
        std::make_unique<Pipeline>(ComputePipelineBuilder()
                                       .set_shader(Engine::get().resources().load<Shader>("assets/shaders/depth_pyramid.slang"))
                                       .build(device));

    init_render_target();
    init_depth_pyramid();

    m_lit_pipeline =
        std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                       .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                       .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                       .set_color_attachment_format(m_render_target.format)
                                       .enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                       .set_depth_format(m_depth_render_target.format)
                                       .set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                       .set_multisampling_none()
                                       .disable_blending()
                                       .set_shader(Engine::get().resources().load<Shader>("assets/shaders/lit.slang"))
                                       .build(device));

    m_mesh_lit_pipeline =
        std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                       .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                       .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                       .set_color_attachment_format(m_render_target.format)
                                       .enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                       .set_depth_format(m_depth_render_target.format)
                                       .set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                       .set_multisampling_none()
                                       .disable_blending()
                                       .set_shader(Engine::get().resources().load<Shader>("assets/shaders/mesh_lit.slang"))
                                       .build(device));

    m_clear_pipeline =
        std::make_unique<Pipeline>(ComputePipelineBuilder()
                                       .set_shader(Engine::get().resources().load<Shader>("assets/shaders/clear.slang"))
                                       .build(device));

    m_debug_line_pipeline =
        std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                       .set_input_topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
                                       .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                       .set_color_attachment_format(m_render_target.format)
                                       .enable_depthtest(false, VK_COMPARE_OP_ALWAYS)
                                       .set_depth_format(m_depth_render_target.format)
                                       .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                       .set_multisampling_none()
                                       .disable_blending()
                                       .set_shader(Engine::get().resources().load<Shader>("assets/shaders/debug_line.slang"))
                                       .build(device));

    m_last_device_extent = device.get_extent();
}

Renderer::~Renderer()
{
    destroy_depth_pyramid();
    destroy_render_target();
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
                                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Renderer::init_depth_pyramid()
{
    Device& device = Engine::get().device();

    const VkExtent2D draw_extent = {
        .width = static_cast<uint32_t>(static_cast<float>(m_render_target.extent.width) * m_render_scale),
        .height = static_cast<uint32_t>(static_cast<float>(m_render_target.extent.height) * m_render_scale)};

    uint32_t width = draw_extent.width / 2;
    uint32_t height = draw_extent.height / 2;

    m_depth_pyramid_levels = 0;
    while (width >= 2 && height >= 2)
    {
        m_depth_pyramid_levels++;

        width /= 2;
        height /= 2;
    }

    m_depth_pyramid =
        device.create_image(VkExtent3D{.width = draw_extent.width / 2, .height = draw_extent.height / 2, .depth = 1},
                            VK_FORMAT_R32_SFLOAT,
                            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                            m_depth_pyramid_levels);

    VkImageViewCreateInfo view_info =
        vk_init::imageview_create_info(VK_FORMAT_R32_SFLOAT, m_depth_pyramid.image, VK_IMAGE_ASPECT_COLOR_BIT);
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    view_info.subresourceRange.levelCount = 1;

    std::vector<PoolSizeRatio> ratios = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
    };

    m_depth_pyramid_allocator.init_pool(device.get(), MAX_DEPTH_PYRAMID_LEVELS * 2, ratios);

    VkSamplerCreateInfo sampler_create_info{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
    sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
    sampler_create_info.minLod = 0;

    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;

    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    VkSamplerReductionModeCreateInfoEXT reduction_create_info = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT};
    reduction_create_info.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;  // min cuz reverse-z!
    sampler_create_info.pNext = &reduction_create_info;

    vkCreateSampler(device.get(), &sampler_create_info, nullptr, &m_depth_pyramid_sampler);

    DescriptorWriter writer;

    for (uint32_t i = 0; i < m_depth_pyramid_levels; ++i)
    {
        view_info.subresourceRange.baseMipLevel = i;

        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VK_CHECK(vkCreateImageView(device.get(), &view_info, nullptr, &m_depth_pyramid_views[i]));

        m_depth_pyramid_sets[i] = m_depth_pyramid_allocator.allocate(m_depth_pyramid_pipeline->get_set_layout(0));
        if (i == 0)
        {
            writer.clear()
                .write_image(0,
                             m_depth_render_target.view,
                             m_depth_pyramid_sampler,
                             VK_IMAGE_LAYOUT_GENERAL,
                             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .write_image(1,
                             m_depth_pyramid_views[i],
                             m_depth_pyramid_sampler,
                             VK_IMAGE_LAYOUT_GENERAL,
                             VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                .update_set(device.get(), m_depth_pyramid_sets[i]);
        }
        else
        {
            writer.clear()
                .write_image(0,
                             m_depth_pyramid_views[i - 1],
                             m_depth_pyramid_sampler,
                             VK_IMAGE_LAYOUT_GENERAL,
                             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .write_image(1,
                             m_depth_pyramid_views[i],
                             VK_NULL_HANDLE,
                             VK_IMAGE_LAYOUT_GENERAL,
                             VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                .update_set(device.get(), m_depth_pyramid_sets[i]);
        }
    }
}

void Renderer::destroy_depth_pyramid() const
{
    Device& device = Engine::get().device();

    for (uint32_t i = 0; i < m_depth_pyramid_levels; ++i) vkDestroyImageView(device.get(), m_depth_pyramid_views[i], nullptr);

    vkDestroySampler(device.get(), m_depth_pyramid_sampler, nullptr);

    m_depth_pyramid_allocator.destroy_pool();
    device.destroy_image(m_depth_pyramid);
}

void Renderer::destroy_render_target() const
{
    Device& device = Engine::get().device();

    device.destroy_image(m_render_target);
    device.destroy_image(m_depth_render_target);
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

    ctx.dcb.draw_auto(static_cast<uint32_t>(frustum_lines.size()), 1, 0, 0);
}

void Renderer::render_debug_visualizations() { render_frustum_lines(); }

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

        destroy_depth_pyramid();
        destroy_render_target();

        init_render_target();
        init_depth_pyramid();
    }

    if (m_last_render_scale != m_render_scale)
    {
        m_last_render_scale = m_render_scale;

        destroy_depth_pyramid();
        init_depth_pyramid();
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
            ctx.dcb.bind_pipeline(m_mesh_lit_pipeline.get());

            VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_mesh_lit_pipeline->get_set_layout(0));
            {
                DescriptorWriter writer;
                writer.write_buffer(0,
                                    scene.get_scene_buffer().buffer,
                                    sizeof(SceneData),
                                    0,
                                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
                writer.write_image(1,
                                   m_depth_pyramid.view,
                                   m_depth_pyramid_sampler,
                                   VK_IMAGE_LAYOUT_GENERAL,
                                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
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
    }

    render_debug_visualizations();

    ctx.dcb.end_rendering();

    ctx.dcb.transition_image(m_depth_render_target.image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    if (!debug_settings.pause_culling)
    {
        ctx.dcb.bind_pipeline(m_depth_pyramid_pipeline.get());
        {
            uint32_t width = m_depth_pyramid.extent.width;
            uint32_t height = m_depth_pyramid.extent.height;

            for (uint32_t mip_index = 0; mip_index < m_depth_pyramid_levels; ++mip_index)
            {
                {
                    VkImageMemoryBarrier write_barrier{};
                    write_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    write_barrier.srcAccessMask = (mip_index == 0) ? 0 : VK_ACCESS_SHADER_READ_BIT;
                    write_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    write_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    write_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    write_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    write_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    write_barrier.image = m_depth_pyramid.image;
                    write_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    write_barrier.subresourceRange.baseMipLevel = mip_index;
                    write_barrier.subresourceRange.levelCount = 1;
                    write_barrier.subresourceRange.baseArrayLayer = 0;
                    write_barrier.subresourceRange.layerCount = 1;

                    ctx.dcb.pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                             0,
                                             0,
                                             nullptr,
                                             0,
                                             nullptr,
                                             1,
                                             &write_barrier);
                }

                ctx.dcb.bind_descriptors(m_depth_pyramid_sets[mip_index], 0, 1);

                uint32_t group_x = (width + 7) / 8;
                uint32_t group_y = (height + 7) / 8;

                ctx.dcb.dispatch(group_x, group_y, 1);

                {
                    VkImageMemoryBarrier read_barrier{};
                    read_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    read_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    read_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    read_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                    read_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    read_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    read_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    read_barrier.image = m_depth_pyramid.image;
                    read_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    read_barrier.subresourceRange.baseMipLevel = mip_index;
                    read_barrier.subresourceRange.levelCount = 1;
                    read_barrier.subresourceRange.baseArrayLayer = 0;
                    read_barrier.subresourceRange.layerCount = 1;

                    ctx.dcb.pipeline_barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                             0,
                                             0,
                                             nullptr,
                                             0,
                                             nullptr,
                                             1,
                                             &read_barrier);
                }

                width /= 2;
                height /= 2;
            }
        }
    }

    ctx.dcb.transition_image(m_render_target.image,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    ctx.dcb.transition_image(video_out, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ctx.dcb.copy_image_to_image(m_render_target.image, video_out, draw_extent, device_extent);
}