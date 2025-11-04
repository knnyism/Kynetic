//
// Created by kennypc on 11/4/25.
//

#pragma once

struct SDL_Window;

namespace kynetic
{
class Device
{
    SDL_Window* m_window{nullptr};
    VkExtent2D m_window_extent{1024, 768};

    VkInstance m_instance{nullptr};
    VkPhysicalDevice m_physical_device{nullptr};
    VkDevice m_device{nullptr};
    VkSurfaceKHR m_surface{nullptr};

    VkDebugUtilsMessengerEXT m_debug_messenger{nullptr};

    class Swapchain* m_swapchain{nullptr};

    int m_frame_count{0};
    bool m_is_minimized{false};
    bool m_is_running{true};

public:
    Device();
    ~Device();

    Device(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;

    bool is_minimized() const;
    bool is_running() const;

    void begin_frame() const;
    void end_frame() const;

    void update();
};
}  // namespace kynetic