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

#include "renderer.hpp"

#include "vk_mem_alloc.h"

using namespace kynetic;

Renderer::Renderer()
{
    Device& device = Engine::get().device();

    init_render_target();

    m_lit_shader = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/lit.slang");
    m_lit_pipeline = std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                                    .set_shader(m_lit_shader)
                                                    .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                    .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                                    .set_color_attachment_format(m_render_target.format)
                                                    .enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                                    .set_depth_format(m_depth_render_target.format)
                                                    .set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                                                    .set_multisampling_none()
                                                    .disable_blending()
                                                    .build(device));
}

Renderer::~Renderer()
{
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

void Renderer::render()
{
    Device& device = Engine::get().device();
    ResourceManager& resources = Engine::get().resources();
    Scene& scene = Engine::get().scene();

    auto& ctx = device.get_context();
    const auto& video_out = device.get_video_out();
    const VkExtent2D device_extent = device.get_extent();

    if (ImGui::Begin("Render Debug"))
    {
        ImGui::SliderFloat("RenderScale", &m_render_scale, 0.3f, 1.f);
        combo_enum(m_render_channel);
        combo_enum(m_rendering_method);
    }
    ImGui::End();

    if (device_extent.width != m_last_device_extent.width || device_extent.height != m_last_device_extent.height)
    {
        m_last_device_extent = device_extent;

        destroy_render_target();
        init_render_target();
    }

    ctx.dcb.transition_image(m_render_target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
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

        ctx.dcb.bind_pipeline(m_lit_pipeline.get());

        DrawPushConstants push_constants;
        push_constants.positions = resources.m_merged_position_buffer_address;
        push_constants.vertices = resources.m_merged_vertex_buffer_address;
        push_constants.instances = scene.get_instance_buffer_address();
        push_constants.materials = resources.m_material_buffer_address;
        ctx.dcb.set_push_constants(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   sizeof(DrawPushConstants),
                                   &push_constants);
        ctx.dcb.bind_index_buffer(resources.m_merged_index_buffer.buffer, VK_INDEX_TYPE_UINT32);

        VkDescriptorSet scene_descriptor = ctx.allocator.allocate(m_lit_pipeline->get_set_layout(0));
        {
            DescriptorWriter writer;
            writer.write_buffer(0, scene.get_scene_buffer().buffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.update_set(device.get(), scene_descriptor);
        }
        ctx.dcb.bind_descriptors(scene_descriptor);
        ctx.dcb.bind_descriptors(device.get_bindless_set(), 1);

        scene.draw(m_rendering_method);
    }
    ctx.dcb.end_rendering();

    ctx.dcb.transition_image(m_render_target.image,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    ctx.dcb.transition_image(video_out, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ctx.dcb.copy_image_to_image(m_render_target.image, video_out, draw_extent, device_extent);
}