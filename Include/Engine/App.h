//
// Created by kenny on 6/30/25.
//

#pragma once

namespace Kynetic {
    class App {
    public:
        GLFWwindow *window{};
        vkb::Instance instance;
        vkb::InstanceDispatchTable instanceDispatchTable;
        VkSurfaceKHR surface{};
        vkb::Device device;
        vkb::DispatchTable disp;
        vkb::Swapchain swapchain;

        void Start();

    private:
        void InitializeGlfw();

        void InitializeVulkan();

        void InitializeImGui();
    };
}
