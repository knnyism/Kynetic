//
// Created by kennypc on 11/4/25.
//

#pragma once

namespace kynetic
{
class Swapchain
{
    friend class Device;

    VkDevice m_device;

    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
    VkFormat m_image_format{VK_FORMAT_B8G8R8A8_UNORM};

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    VkExtent2D m_extent{};

    uint32_t m_image_index{0};

    [[nodiscard]] const VkImage& get_video_out() const { return m_images[m_image_index]; };
    [[nodiscard]] const VkImageView& get_image_view() const { return m_image_views[m_image_index]; };

    VkResult acquire_next_image(VkSemaphore semaphore);
    void init(VkPhysicalDevice physical_device, VkSurfaceKHR surface, uint32_t width, uint32_t height);
    void shutdown() const;

public:
    Swapchain(VkDevice device);
};
}  // namespace kynetic
