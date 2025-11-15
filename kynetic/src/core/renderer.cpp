//
// Created by kennypc on 11/5/25.
//

#include "device.hpp"
#include "engine.hpp"
#include "resource_manager.hpp"

#include "renderer.hpp"

#include "imgui.h"

#include "rendering/command_buffer.hpp"
#include "rendering/pipeline.hpp"
#include "rendering/shader.hpp"
#include "rendering/mesh.hpp"
#include "rendering/model.hpp"

using namespace kynetic;

Renderer::Renderer()
{
    Device& device = Engine::get().device();

    init_render_target();

    m_gradient = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/gradient.slang");
    m_gradient_descriptor_set = device.get_descriptor_allocator().allocate(device.get(), m_gradient->get_layout_at(0));

    m_triangle_frag = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/triangle.frag.slang");
    m_triangle_vert = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/triangle.vert.slang");

    m_mesh_pipeline = std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                                     .set_shader(m_triangle_vert, m_triangle_frag)
                                                     .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                     .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                                     .set_color_attachment_format(m_render_target.format)
                                                     .enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                                                     .set_depth_format(m_depth_render_target.format)
                                                     .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                                                     .set_multisampling_none()
                                                     .disable_blending()
                                                     .build(device.get()));

    Effect& gradient = backgroundEffects.emplace_back(
        "gradient",
        std::make_unique<Pipeline>(ComputePipelineBuilder().set_shader(m_gradient).build(device.get())));
    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
    gradient.data.data2 = glm::vec4(0, 0, 1, 1);

    Effect& sky = backgroundEffects.emplace_back(
        "sky",
        std::make_unique<Pipeline>(ComputePipelineBuilder().set_shader(m_gradient).build(device.get())));
    sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

    m_quad = Engine::get().resources().load<Model>("assets/shared_assets/models/basicmesh.glb");

    update_gradient_descriptor();
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

void Renderer::update_gradient_descriptor() const
{
    VkDescriptorImageInfo image_info;
    image_info.sampler = VK_NULL_HANDLE;
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_info.imageView = m_render_target.view;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.dstSet = m_gradient_descriptor_set;
    write.dstBinding = 0;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(Engine::get().device().get(), 1, &write, 0, nullptr);
}

void Renderer::render()
{
    Device& device = Engine::get().device();
    auto& ctx = device.get_context();
    const auto& video_out = device.get_video_out();
    const VkExtent2D device_extent = device.get_extent();

    if (ImGui::Begin("background"))
    {
        Effect& selected = backgroundEffects[static_cast<size_t>(currentBackgroundEffect)];
        ImGui::Text("Selected effect: %s", selected.name);

        ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0, static_cast<int>(backgroundEffects.size()) - 1);

        ImGui::InputFloat4("data1", reinterpret_cast<float*>(&selected.data.data1));
        ImGui::InputFloat4("data2", reinterpret_cast<float*>(&selected.data.data2));
        ImGui::InputFloat4("data3", reinterpret_cast<float*>(&selected.data.data3));
        ImGui::InputFloat4("data4", reinterpret_cast<float*>(&selected.data.data4));

        ImGui::SliderFloat("Render Scale", &m_render_scale, 0.3f, 1.f);
    }
    ImGui::End();

    if (device_extent.width != m_last_device_extent.width || device_extent.height != m_last_device_extent.height)
    {
        m_last_device_extent = device_extent;

        destroy_render_target();
        init_render_target();

        update_gradient_descriptor();
    }

    const VkExtent2D draw_extent = {
        .width = static_cast<uint32_t>(static_cast<float>(m_render_target.extent.width) * m_render_scale),
        .height = static_cast<uint32_t>(static_cast<float>(m_render_target.extent.height) * m_render_scale)};

    ctx.dcb.transition_image(m_render_target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    Effect& effect = backgroundEffects[static_cast<size_t>(currentBackgroundEffect)];
    effect.data.size = glm::ivec2(draw_extent.width, draw_extent.height);

    ctx.dcb.bind_pipeline(effect.pipeline.get());
    ctx.dcb.bind_descriptors(m_gradient_descriptor_set);
    ctx.dcb.set_push_constants(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(GradientPushConstants), &effect.data);
    ctx.dcb.dispatch(static_cast<uint32_t>(std::ceilf(static_cast<float>(draw_extent.width) / 16.0f)),
                     static_cast<uint32_t>(std::ceilf(static_cast<float>(draw_extent.height) / 16.0f)),
                     1);

    ctx.dcb.transition_image(m_render_target.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    ctx.dcb.transition_image(m_depth_render_target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo color_attachment =
        vk_init::attachment_info(m_render_target.view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depth_attachment =
        vk_init::depth_attachment_info(m_depth_render_target.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo render_info = vk_init::rendering_info(draw_extent, &color_attachment, &depth_attachment);
    ctx.dcb.begin_rendering(render_info);

    ctx.dcb.set_viewport(static_cast<float>(draw_extent.width), static_cast<float>(draw_extent.height));
    ctx.dcb.set_scissor(draw_extent.width, draw_extent.height);

    glm::mat4 view = glm::lookAt(
        glm::vec3(sinf(static_cast<float>(m_frame_count) / 100), -0.5f, cosf(static_cast<float>(m_frame_count) / 100)) * 5.f,
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0));
    glm::mat4 projection = glm::perspective(glm::radians(85.f),
                                            static_cast<float>(draw_extent.width) / static_cast<float>(draw_extent.height),
                                            0.1f,
                                            100.f);
    projection[1][1] *= -1.f;

    DrawPushConstants push_constants;
    push_constants.world_matrix = glm::transpose(projection * view);
    push_constants.vertex_buffer = m_quad->get_meshes()[2]->get_address();

    ctx.dcb.bind_pipeline(m_mesh_pipeline.get());
    ctx.dcb.set_push_constants(VK_SHADER_STAGE_VERTEX_BIT, sizeof(DrawPushConstants), &push_constants);
    ctx.dcb.bind_index_buffer(m_quad->get_meshes()[2]->get_indices(), m_quad->get_meshes()[2]->get_index_type());
    for (auto& primitive : m_quad->get_meshes()[2]->get_primitives()) ctx.dcb.draw(primitive.count, 1, primitive.first, 0, 0);

    ctx.dcb.end_rendering();

    ctx.dcb.transition_image(m_render_target.image,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    ctx.dcb.transition_image(video_out, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ctx.dcb.copy_image_to_image(m_render_target.image, video_out, draw_extent, device_extent);

    m_frame_count++;
}