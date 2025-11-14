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

#include "shader_types.hpp"

using namespace kynetic;

Renderer::Renderer()
{
    Device& device = Engine::get().device();
    m_draw_image = device.create_image(device.get_extent(),
                                       VK_FORMAT_R16G16B16A16_SFLOAT,
                                       VMA_MEMORY_USAGE_GPU_ONLY,
                                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       VK_IMAGE_ASPECT_COLOR_BIT);

    m_deletion_queue.push_function([this, &device]() { device.destroy_image(m_draw_image); });

    m_gradient = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/gradient.slang");

    m_triangle_frag = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/triangle.frag.slang");
    m_triangle_vert = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/triangle.vert.slang");

    m_mesh_pipeline = std::make_unique<Pipeline>(GraphicsPipelineBuilder()
                                                     .set_shader(m_triangle_vert, m_triangle_frag)
                                                     .set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                                                     .set_polygon_mode(VK_POLYGON_MODE_FILL)
                                                     .set_color_attachment_format(m_draw_image.format)
                                                     .set_depth_format(VK_FORMAT_UNDEFINED)
                                                     .set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                                                     .set_multisampling_none()
                                                     .disable_blending()
                                                     .disable_depthtest()
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

    m_gradient_descriptor_set = device.get_descriptor_allocator().allocate(device.get(), m_gradient->get_layout_at(0));

    VkDescriptorImageInfo image_info;
    image_info.sampler = VK_NULL_HANDLE;
    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_info.imageView = m_draw_image.view;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.dstSet = m_gradient_descriptor_set;
    write.dstBinding = 0;
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(device.get(), 1, &write, 0, nullptr);

    std::array<Vertex, 4> rect_vertices;

    rect_vertices[0].position = {0.5, -0.5, 0};
    rect_vertices[1].position = {0.5, 0.5, 0};
    rect_vertices[2].position = {-0.5, -0.5, 0};
    rect_vertices[3].position = {-0.5, 0.5, 0};

    rect_vertices[0].color = {0, 0, 0, 1};
    rect_vertices[1].color = {0.5, 0.5, 0.5, 1};
    rect_vertices[2].color = {1, 0, 0, 1};
    rect_vertices[3].color = {0, 1, 0, 1};

    std::array<uint32_t, 6> rect_indices;

    rect_indices[0] = 0;
    rect_indices[1] = 1;
    rect_indices[2] = 2;

    rect_indices[3] = 2;
    rect_indices[4] = 1;
    rect_indices[5] = 3;

    m_quad = Engine::get().resources().load<Mesh>("", rect_indices, rect_vertices);
}

Renderer::~Renderer() { m_deletion_queue.flush(); }

void Renderer::render()
{
    static VkExtent2D draw_extent = {.width = m_draw_image.extent.width, .height = m_draw_image.extent.height};

    if (ImGui::Begin("background"))
    {
        Effect& selected = backgroundEffects[static_cast<size_t>(currentBackgroundEffect)];
        ImGui::Text("Selected effect: %s", selected.name);

        ImGui::SliderInt("Effect Index", &currentBackgroundEffect, 0, static_cast<int>(backgroundEffects.size()) - 1);

        ImGui::InputFloat4("data1", reinterpret_cast<float*>(&selected.data.data1));
        ImGui::InputFloat4("data2", reinterpret_cast<float*>(&selected.data.data2));
        ImGui::InputFloat4("data3", reinterpret_cast<float*>(&selected.data.data3));
        ImGui::InputFloat4("data4", reinterpret_cast<float*>(&selected.data.data4));
    }
    ImGui::End();

    ImGui::Render();

    Device& device = Engine::get().device();
    auto& ctx = device.get_context();
    const auto& render_target = device.get_render_target();

    ctx.dcb.transition_image(m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    const Effect& effect = backgroundEffects[static_cast<size_t>(currentBackgroundEffect)];

    ctx.dcb.bind_pipeline(effect.pipeline.get());
    ctx.dcb.bind_descriptors(m_gradient_descriptor_set);
    ctx.dcb.set_push_constants(VK_SHADER_STAGE_COMPUTE_BIT, sizeof(ComputePushConstants), &effect.data);
    ctx.dcb.dispatch(static_cast<uint32_t>(std::ceilf(static_cast<float>(m_draw_image.extent.width) / 16.0f)),
                     static_cast<uint32_t>(std::ceilf(static_cast<float>(m_draw_image.extent.height) / 16.0f)),
                     1);

    ctx.dcb.transition_image(m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo color_attachment =
        vk_init::attachment_info(m_draw_image.view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingInfo render_info = vk_init::rendering_info(draw_extent, &color_attachment, nullptr);
    ctx.dcb.begin_rendering(render_info);

    ctx.dcb.set_viewport(static_cast<float>(draw_extent.width), static_cast<float>(draw_extent.height));
    ctx.dcb.set_scissor(draw_extent.width, draw_extent.height);

    DrawPushConstants push_constants;
    push_constants.world_matrix = glm::mat4{1.f};
    push_constants.vertex_buffer = m_quad->get_address();

    ctx.dcb.bind_pipeline(m_mesh_pipeline.get());
    ctx.dcb.set_push_constants(VK_SHADER_STAGE_VERTEX_BIT, sizeof(DrawPushConstants), &push_constants);
    ctx.dcb.bind_index_buffer(m_quad->get_indices(), m_quad->get_index_type());
    ctx.dcb.draw(6, 1, 0, 0, 0);

    ctx.dcb.end_rendering();

    ctx.dcb.transition_image(m_draw_image.image,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    ctx.dcb.transition_image(render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ctx.dcb.copy_image_to_image(m_draw_image.image, render_target, draw_extent, device.get_extent());

    m_frame_count++;
}