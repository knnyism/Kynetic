//
// Created by kenny on 3-7-25.
//

#pragma once

namespace Kynetic {

class Window {
public:
    Window(int width, int height, const char *title);
    ~Window();

    static void poll_events();
    [[nodiscard]] bool should_close() const;

    [[nodiscard]] GLFWwindow* get_handle() const { return m_window; }
    VkSurfaceKHR create_surface(VkInstance instance, const VkAllocationCallbacks* allocator = nullptr) const;

    // void GetFramebufferSize(int* width, int* height) const;
private:
    GLFWwindow* m_window = nullptr;

    // static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
};

} // Kynetic