//
// Created by kenny on 3-7-25.
//

#pragma once

namespace Kynetic {

class Device;

class Swapchain {
    const Device& m_device;
    vkb::Swapchain m_swapchain;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;

    std::vector<VkSemaphore> m_available_semaphores;
    std::vector<VkSemaphore> m_finished_semaphores;
    std::vector<VkFence> m_in_flight_fences;
    std::vector<VkFence> m_image_in_flight;

    size_t m_frame_index = 0;

    void create_sync_objects();
public:
    explicit Swapchain(const Device& device);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    [[nodiscard]] VkSwapchainKHR get() const { return m_swapchain.swapchain; }
    [[nodiscard]] const std::vector<VkImage>& get_images() const { return m_images; }
    [[nodiscard]] const std::vector<VkImageView>& get_image_views() const { return m_image_views; }
    [[nodiscard]] VkFormat get_image_format() const { return m_swapchain.image_format; }
    [[nodiscard]] VkExtent2D get_extent() const { return m_swapchain.extent; }
    [[nodiscard]] uint32_t get_image_count() const { return m_swapchain.image_count; }

    [[nodiscard]] VkSemaphore get_available_semaphore() const { return m_available_semaphores[m_frame_index]; }
    [[nodiscard]] VkSemaphore get_finished_semaphore(const uint32_t image_index) const { return m_finished_semaphores[image_index]; }
    [[nodiscard]] VkFence get_in_flight_fence() const { return m_in_flight_fences[m_frame_index]; }

    void reset_fence() const;

    uint32_t acquire_next_image_index();
    void present(uint32_t image_index);

    void create_swapchain();
};

} // Kynetic
