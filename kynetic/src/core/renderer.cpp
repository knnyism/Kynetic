//
// Created by kennypc on 11/5/25.
//

#include "device.hpp"
#include "engine.hpp"
#include "renderer.hpp"

using namespace kynetic;

Renderer::Renderer()
{
    Device& device = Engine::get().get_device();
    m_draw_image = device.create_image(device.get_extent(),
                                       VK_FORMAT_R16G16B16A16_SFLOAT,
                                       VMA_MEMORY_USAGE_GPU_ONLY,
                                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       VK_IMAGE_ASPECT_COLOR_BIT);

    m_deletion_queue.push_function([this, &device]() { device.destroy_image(m_draw_image); });
}
Renderer::~Renderer() { m_deletion_queue.flush(); }

void Renderer::draw_background() const
{
    const auto& ctx = Engine::get().get_device().get_context();

    VkClearColorValue clear_value;
    const float flash = std::abs(std::sin(static_cast<float>(m_frame_count) / 120.f));
    clear_value = {{0.0f, 0.0f, flash, 1.0f}};

    const VkImageSubresourceRange clear_range = vk_init::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdClearColorImage(ctx.dcb, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);
}
void Renderer::render()
{
    Device& device = Engine::get().get_device();
    const auto& ctx = device.get_context();

    vk_util::transition_image(ctx.dcb, m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    draw_background();
    vk_util::transition_image(ctx.dcb, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    const auto& render_target = device.get_render_target();
    vk_util::transition_image(ctx.dcb, render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vk_util::copy_image_to_image(ctx.dcb,
                                 m_draw_image.image,
                                 render_target,
                                 {.width = m_draw_image.extent.width, .height = m_draw_image.extent.height},
                                 device.get_extent());
    vk_util::transition_image(ctx.dcb, render_target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    m_frame_count++;
}