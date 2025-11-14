//
// Created by kennypc on 11/4/25.
//

#pragma once

#include "rendering/command_buffer.hpp"

struct SDL_Window;

namespace kynetic
{
struct Context
{
    CommandBuffer dcb;

    VkSemaphore swapchain_semaphore, render_semaphore;
    VkFence render_fence;

    DeletionQueue deletion_queue;
};

struct QueueIndices
{
    uint32_t graphics;
    uint32_t present;
};

constexpr uint8_t MAX_FRAMES_IN_FLIGHT = 3;

class Device
{
    friend class Engine;

    SDL_Window* m_window{nullptr};
    VkExtent2D m_window_extent{1024, 768};

    VkInstance m_instance{nullptr};
    VkPhysicalDevice m_physical_device{nullptr};
    VkDevice m_device{nullptr};
    VkSurfaceKHR m_surface{nullptr};

    VkDebugUtilsMessengerEXT m_debug_messenger{nullptr};

    QueueIndices m_queue_indices;
    VkQueue m_graphics_queue;
    VkQueue m_present_queue;

    VmaAllocator m_allocator;

    Context m_ctxs[MAX_FRAMES_IN_FLIGHT];

    class Swapchain* m_swapchain{nullptr};

    DescriptorAllocator m_descriptor_allocator;

    VkFence m_immediate_fence;
    VkCommandPool m_immediate_command_pool;
    VkCommandBuffer m_immediate_command_buffer;

    VkDescriptorPool imgui_descriptor_pool;

    Slang::ComPtr<slang::IGlobalSession> m_slang_session;

    int m_frame_count{0};
    bool m_is_minimized{false};
    bool m_is_running{true};

    void wait_until_safe_for_rendering() const;

    void begin_frame();
    void end_frame();

    void update();

public:
    Device();
    ~Device();

    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;

    [[nodiscard]] const VkDevice& get() const { return m_device; }
    [[nodiscard]] VkExtent2D get_extent() const { return m_window_extent; }

    [[nodiscard]] Context& get_context() { return m_ctxs[m_frame_count % MAX_FRAMES_IN_FLIGHT]; }
    [[nodiscard]] const Context& get_context() const { return m_ctxs[m_frame_count % MAX_FRAMES_IN_FLIGHT]; }

    [[nodiscard]] Slang::ComPtr<slang::IGlobalSession>& get_slang_session() { return m_slang_session; }
    [[nodiscard]] const Slang::ComPtr<slang::IGlobalSession>& get_slang_session() const { return m_slang_session; }

    [[nodiscard]] const DescriptorAllocator& get_descriptor_allocator() const { return m_descriptor_allocator; }

    [[nodiscard]] const VkImage& get_render_target() const;

    bool is_minimized() const;
    bool is_running() const;

    AllocatedImage create_image(VkExtent3D extent,
                                VkFormat format,
                                VmaMemoryUsage usage,
                                VkImageUsageFlags usage_flags,
                                VkMemoryPropertyFlags property_flags,
                                VkImageAspectFlags aspect_flags) const;

    AllocatedImage create_image(VkExtent2D extent,
                                VkFormat format,
                                VmaMemoryUsage usage,
                                VkImageUsageFlags usage_flags,
                                VkMemoryPropertyFlags property_flags,
                                VkImageAspectFlags aspect_flags) const;

    void destroy_image(const AllocatedImage& image) const;

    AllocatedBuffer create_buffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) const;
    void destroy_buffer(const AllocatedBuffer& buffer) const;

    void wait_idle() const;
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) const;
};
}  // namespace kynetic