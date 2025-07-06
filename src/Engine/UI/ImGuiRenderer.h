//
// Created by kenny on 5-7-25.
//

#pragma once

namespace Kynetic {

class Window;
class Device;
class Swapchain;

class ImGuiRenderer {
public:
    ImGuiRenderer(const Window &window, const Device &device, const Swapchain &swapchain, VkRenderPass render_pass);
    ~ImGuiRenderer();

    static void new_frame();
    static void render();
    static void render_draw_data(const VkCommandBuffer &cmd_buf);
};

} // Kynetic
