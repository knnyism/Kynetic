//
// Created by kenny on 11/5/25.
//

#include "imgui.h"

#include "device.hpp"
#include "engine.hpp"
#include "resource_manager.hpp"

#include "renderer.hpp"

#include "rendering/descriptor_writer.hpp"
#include "rendering/pipeline.hpp"
#include "rendering/shader.hpp"
#include "rendering/mesh.hpp"
#include "rendering/model.hpp"

#include "vk_mem_alloc.h"
#include "rendering/texture.hpp"

using namespace kynetic;

Renderer::Renderer()
{
    Device& device = Engine::get().device();

    init_render_target();

    m_gradient = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/gradient.slang");

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
    }

    const VkExtent2D draw_extent = {
        .width = static_cast<uint32_t>(static_cast<float>(m_render_target.extent.width) * m_render_scale),
        .height = static_cast<uint32_t>(static_cast<float>(m_render_target.extent.height) * m_render_scale)};

    ctx.dcb.transition_image(m_render_target.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    Effect& effect = backgroundEffects[static_cast<size_t>(currentBackgroundEffect)];
    effect.data.size = glm::ivec2(draw_extent.width, draw_extent.height);

    ctx.dcb.bind_pipeline(effect.pipeline.get());
    {
        VkDescriptorSet gradient_descriptor = ctx.allocator.allocate(effect.pipeline->get_set_layout(0));

        DescriptorWriter writer;
        writer.write_image(0, m_render_target.view, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.update_set(device.get(), gradient_descriptor);
        ctx.dcb.bind_descriptors(gradient_descriptor);
    }
    ctx.dcb.set_push_constants(ShaderStage::Compute, sizeof(GradientPushConstants), &effect.data);
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

    float lol = sinf(static_cast<float>(m_frame_count) / 60.f);

    glm::mat4 view = glm::translate(glm::vec3{0, 0, -5}) * glm::rotate(glm::radians(90.0f * lol), glm::vec3{0, 1, 0});
    glm::mat4 projection = glm::perspective(glm::radians(70.f),
                                            static_cast<float>(draw_extent.width) / static_cast<float>(draw_extent.height),
                                            1000.f,
                                            0.1f);
    projection[1][1] *= -1;

    DrawPushConstants push_constants;
    push_constants.vertex_buffer = m_quad->get_meshes()[2]->get_address();

    ctx.dcb.bind_pipeline(m_mesh_pipeline.get());
    {
        AllocatedBuffer scene_data_buffer =
            device.create_buffer(sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(scene_data_buffer); });

        void* mapped_data;
        vmaMapMemory(device.get_allocator(), scene_data_buffer.allocation, &mapped_data);
        auto* scene_uniform_data = static_cast<SceneData*>(mapped_data);
        scene_uniform_data->view = view;
        scene_uniform_data->proj = projection;
        scene_uniform_data->vp = projection * view;
        scene_uniform_data->ambient_color = glm::vec4(0.1f, 0.1f, 0.1f, 0.f);
        scene_uniform_data->sun_direction = glm::vec4(0.f, -1.f, 0.f, 0.f);
        scene_uniform_data->sun_color = glm::vec4(0.5f, 0.5f, 0.5f, 0.f);
        vmaUnmapMemory(device.get_allocator(), scene_data_buffer.allocation);

        VkDescriptorSet mesh_descriptor = ctx.allocator.allocate(m_mesh_pipeline->get_set_layout(0));
        {
            DescriptorWriter writer;
            writer.write_buffer(0, scene_data_buffer.buffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            auto texture = Engine::get().resources().find<Texture>("dev/missing");
            writer.write_image(1,
                               texture->m_image.view,
                               texture->m_sampler,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.update_set(device.get(), mesh_descriptor);
        }

        ctx.dcb.bind_descriptors(mesh_descriptor);
    }

    ctx.dcb.set_push_constants(ShaderStage::Vertex, sizeof(DrawPushConstants), &push_constants);
    ctx.dcb.bind_index_buffer(m_quad->get_meshes()[2]->get_indices(), m_quad->get_meshes()[2]->get_index_type());
    for (const auto& primitive : m_quad->get_meshes()[2]->get_primitives())
        ctx.dcb.draw(primitive.count, 1, primitive.first, 0, 0);

    ctx.dcb.end_rendering();

    ctx.dcb.transition_image(m_render_target.image,
                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    ctx.dcb.transition_image(video_out, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ctx.dcb.copy_image_to_image(m_render_target.image, video_out, draw_extent, device_extent);

    m_frame_count++;
}