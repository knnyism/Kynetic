//
// Created by kenny on 3-7-25.
//

#pragma once

namespace Kynetic {

class Window {
    GLFWwindow* m_window = nullptr;
public:
    Window(int width, int height, const char *title);
    ~Window();

    static void poll_events();
    [[nodiscard]] bool should_close() const;

    [[nodiscard]] GLFWwindow* get() const { return m_window; }

    void set_user_pointer(void* user_pointer) const;
    void set_resize_callback(GLFWframebuffersizefun framebuffer_resize_callback) const;
    // void GetFramebufferSize(int* width, int* height) const;
};

} // Kynetic