//
// Created by kenny on 6/30/25.
//

#pragma once

namespace Kynetic {

    class Window;
    class Device;
    class Swapchain;
    class Renderer;

    class App {
    public:
        App();
        ~App();

        void Start();
    private:
        std::unique_ptr<Window> m_window;
        std::unique_ptr<Device> m_device;
        std::unique_ptr<Swapchain> m_swapchain;
        std::unique_ptr<Renderer> m_renderer;

        bool m_window_resized = false;

        void recreate_swapchain() const;

        int DrawFrame();

        int InitializeVulkan();
        void InitializeImGui() const;

        void InitializeCallbacks();

        void Cleanup();
    };

} // Kynetic