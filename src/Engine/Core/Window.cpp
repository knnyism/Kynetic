//
// Created by kenny on 3-7-25.
//

#include "Window.h"

namespace Kynetic {

Window::Window(int width, int height, const char *title) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(m_window);
}

void Window::poll_events() {
    glfwPollEvents();
}

bool Window::should_close() const {
    return glfwWindowShouldClose(m_window);
}

VkSurfaceKHR Window::create_surface(VkInstance instance, const VkAllocationCallbacks* allocator) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, m_window, allocator, &surface) != VK_SUCCESS) {
        const char* error_message;
        if (int result = glfwGetError(&error_message); result != 0) {
            std::println("{} {}", result, error_message);
        }
        surface = VK_NULL_HANDLE;
    }
    return surface;
}

} // Kynetic