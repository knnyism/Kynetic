//
// Created by kennypc on 11/4/25.
//

#include "swapchain.hpp"

#include "VkBootstrap.h"

using namespace kynetic;

Swapchain::Swapchain(const VkDevice device,
                     const VkPhysicalDevice physical_device,
                     const VkSurfaceKHR surface,
                     const uint32_t width,
                     const uint32_t height)
    : m_device(device), m_image_format(VK_FORMAT_B8G8R8A8_UNORM)
{
    vkb::Swapchain swapchain =
        vkb::SwapchainBuilder(physical_device, device, surface)
            .set_desired_format(VkSurfaceFormatKHR{.format = m_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_extent(width, height)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    m_extent = swapchain.extent;
    m_swapchain = swapchain.swapchain;
    m_images = swapchain.get_images().value();
    m_image_views = swapchain.get_image_views().value();
}
Swapchain::~Swapchain()
{
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    for (const auto image_view : m_image_views) vkDestroyImageView(m_device, image_view, nullptr);
}