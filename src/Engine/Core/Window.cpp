//
// Created by kenny on 3-7-25.
//

#include "Window.h"

namespace Kynetic {

Window::Window(const int width, const int height, const char *title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialise GLFW");
    }

    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);

    if (m_window == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::poll_events() {
    glfwPollEvents();
}

bool Window::should_close() const {
    return glfwWindowShouldClose(m_window);
}

void Window::set_user_pointer(void *user_pointer) const {
    glfwSetWindowUserPointer(m_window, user_pointer);
}

void Window::set_resize_callback(const GLFWframebuffersizefun framebuffer_resize_callback) const {
    glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
}

} // Kynetic