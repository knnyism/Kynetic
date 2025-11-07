//
// Created by kennypc on 11/5/25.
//

#include "rendering/shader.hpp"

#include "device.hpp"
#include "engine.hpp"
#include "resource_manager.hpp"

#include "renderer.hpp"

#include "imgui.h"

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

    backgroundEffects.push_back({"gradient",
                                 std::make_unique<Shader>(device.get(),
                                                          Engine::get().resources().load<ShaderResource>(
                                                              "assets/shared_assets/shaders/gradient.slang"))});
    backgroundEffects[0].data.data1 = glm::vec4(1, 0, 0, 1);
    backgroundEffects[0].data.data2 = glm::vec4(0, 0, 1, 1);

    backgroundEffects.push_back({"sky",
                                 std::make_unique<Shader>(device.get(),
                                                          Engine::get().resources().load<ShaderResource>(
                                                              "assets/shared_assets/shaders/gradient.slang"))});
    backgroundEffects[1].data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

    backgroundEffects[0].shader->set_image("image", m_draw_image.view);
    backgroundEffects[1].shader->set_image("image", m_draw_image.view);
}

Renderer::~Renderer() { m_deletion_queue.flush(); }

void Renderer::render()
{
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
    const auto& ctx = device.get_context();
    const auto& render_target = device.get_render_target();

    vk_util::transition_image(ctx.dcb, m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    const Effect& effect = backgroundEffects[static_cast<size_t>(currentBackgroundEffect)];

    effect.shader->set_push_constants(ctx.dcb, sizeof(ComputePushConstants), &effect.data);
    effect.shader->dispatch(ctx.dcb,
                            static_cast<uint32_t>(std::ceilf(static_cast<float>(m_draw_image.extent.width) / 16.0f)),
                            static_cast<uint32_t>(std::ceilf(static_cast<float>(m_draw_image.extent.height) / 16.0f)),
                            1);

    vk_util::transition_image(ctx.dcb, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vk_util::transition_image(ctx.dcb, render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vk_util::copy_image_to_image(ctx.dcb,
                                 m_draw_image.image,
                                 render_target,
                                 {.width = m_draw_image.extent.width, .height = m_draw_image.extent.height},
                                 device.get_extent());

    m_frame_count++;
}