//
// Created by kennypc on 11/4/25.
//

#include "VkBootstrap.h"

#include "swapchain.hpp"

using namespace kynetic;

Swapchain::Swapchain(VkDevice device,
                     VkPhysicalDevice physical_device,
                     VkSurfaceKHR surface,
                     const uint32_t width,
                     const uint32_t height)
    : m_device(device), m_image_format(VK_FORMAT_B8G8R8A8_UNORM), m_image_index(0)
{
    vkb::Swapchain swapchain =
        vkb::SwapchainBuilder(physical_device, device, surface)
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

Swapchain::~Swapchain()
{
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    for (auto* image_view : m_image_views) vkDestroyImageView(m_device, image_view, nullptr);
}

Swapchain::Swapchain(Swapchain&& other) noexcept
    : m_device(other.m_device),
      m_swapchain(other.m_swapchain),
      m_image_format(other.m_image_format),
      m_images(std::move(other.m_images)),
      m_image_views(std::move(other.m_image_views)),
      m_extent(other.m_extent),
      m_image_index(other.m_image_index)
{
    other.m_swapchain = VK_NULL_HANDLE;
    other.m_image_views.clear();
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
{
    if (this != &other)
    {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        for (auto* image_view : m_image_views) vkDestroyImageView(m_device, image_view, nullptr);

        m_device = other.m_device;
        m_swapchain = other.m_swapchain;
        m_image_format = other.m_image_format;
        m_images = std::move(other.m_images);
        m_image_views = std::move(other.m_image_views);
        m_extent = other.m_extent;
        m_image_index = other.m_image_index;

        // Nullify the moved-from object
        other.m_swapchain = VK_NULL_HANDLE;
        other.m_image_views.clear();
    }
    return *this;
}

VkResult Swapchain::swap(VkSemaphore semaphore)
{
    return vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, semaphore, nullptr, &m_image_index);
}