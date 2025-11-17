//
// Created by kenny on 11/4/25.
//

#include "VkBootstrap.h"

#include "swapchain.hpp"

using namespace kynetic;

Swapchain::Swapchain(VkDevice device) : m_device(device) {}

VkResult Swapchain::acquire_next_image(VkSemaphore semaphore)
{
    return vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, semaphore, nullptr, &m_image_index);
}

void Swapchain::init(VkPhysicalDevice physical_device, VkSurfaceKHR surface, const uint32_t width, const uint32_t height)
{
    vkb::Swapchain swapchain =
        vkb::SwapchainBuilder(physical_device, m_device, surface)
            .set_desired_format(VkSurfaceFormatKHR{.format = m_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    m_extent = swapchain.extent;
    m_swapchain = swapchain.swapchain;
    m_images = swapchain.get_images().value();
    m_image_views = swapchain.get_image_views().value();
}

void Swapchain::shutdown() const
{
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    for (auto* image_view : m_image_views) vkDestroyImageView(m_device, image_view, nullptr);
}