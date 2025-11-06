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

    VkSwapchainKHR m_swapchain;
    VkFormat m_image_format;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    VkExtent2D m_extent;

    uint32_t m_image_index;

    Swapchain(VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, uint32_t width, uint32_t height);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;

    [[nodiscard]] const VkImage& get_image() const { return m_images[m_image_index]; };
    [[nodiscard]] const VkImageView& get_image_view() const { return m_image_views[m_image_index]; };

    void swap(VkSemaphore semaphore);
};
}  // namespace kynetic
