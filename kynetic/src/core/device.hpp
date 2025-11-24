//
// Created by kenny on 11/4/25.
//

#pragma once

#include "rendering/command_buffer.hpp"
#include "rendering/descriptor_allocator.hpp"

struct SDL_Window;

namespace kynetic
{
struct Context
{
    CommandBuffer dcb;
    DescriptorAllocatorGrowable allocator;
    DeletionQueue deletion_queue;
};

struct QueueIndices
{
    uint32_t graphics;
    uint32_t present;
};

constexpr uint8_t MAX_FRAMES_IN_FLIGHT = 4;

class Device
{
    friend class Engine;

    struct Sync
    {
        VkSemaphore image_available;
        VkSemaphore render_finished;
        VkFence in_flight_fence;
    };

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
    std::vector<Sync> m_syncs;

    std::unique_ptr<class Swapchain> m_swapchain;

    VkFence m_immediate_fence;

    CommandBuffer m_immediate_command_buffer;

    VkDescriptorPool imgui_descriptor_pool;

    Slang::ComPtr<slang::IGlobalSession> m_slang_session;

    uint32_t m_frame_count{0};
    bool m_is_minimized{false};
    bool m_is_running{true};
    bool m_resize_requested{false};

    void resize_swapchain();

    bool begin_frame();
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

    [[nodiscard]] SDL_Window& get_window() const { return *m_window; }
    [[nodiscard]] VkExtent2D get_extent() const { return m_window_extent; }

    [[nodiscard]] Context& get_context() { return m_ctxs[m_frame_count % MAX_FRAMES_IN_FLIGHT]; }
    [[nodiscard]] const Context& get_context() const { return m_ctxs[m_frame_count % MAX_FRAMES_IN_FLIGHT]; }

    [[nodiscard]] Slang::ComPtr<slang::IGlobalSession>& get_slang_session() { return m_slang_session; }
    [[nodiscard]] const Slang::ComPtr<slang::IGlobalSession>& get_slang_session() const { return m_slang_session; }

    [[nodiscard]] const VkImage& get_video_out() const;

    [[nodiscard]] VmaAllocator get_allocator() const { return m_allocator; };

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

    AllocatedImage create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false) const;
    AllocatedImage create_image(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);

    void destroy_image(const AllocatedImage& image) const;

    AllocatedBuffer create_buffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) const;
    void destroy_buffer(const AllocatedBuffer& buffer) const;

    void wait_idle() const;
    void immediate_submit(std::function<void(CommandBuffer& cmd)>&& function);
};
}  // namespace kynetic