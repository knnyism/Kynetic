//
// Created by kennypc on 11/4/25.
//

#pragma once

namespace kynetic
{

class Swapchain
{
    const VkDevice m_device;

    VkSwapchainKHR m_swapchain;
    VkFormat m_image_format;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    VkExtent2D m_extent;

public:
    Swapchain(VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, uint32_t width, uint32_t height);
    ~Swapchain();
};
}  // namespace kynetic
