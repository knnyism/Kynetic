//
// Created by kenny on 6/30/25.
//

#pragma once
#include "Engine/Renderer/Renderer.h"

namespace Kynetic {

class Window;

class App: public Renderer {
    std::unique_ptr<Window> m_window;
    std::unique_ptr<ImGuiRenderer> m_imgui_renderer;

    bool m_window_resized = false;

    void recreate_swapchain() const;
    int draw_frame();
    void initialize_callbacks();
public:
    App();
    ~App();

    void start();
};

} // Kynetic