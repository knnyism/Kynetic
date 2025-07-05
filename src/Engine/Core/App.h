//
// Created by kenny on 6/30/25.
//

#pragma once

struct RenderData {
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};

namespace Kynetic {

    class Window;
    class Device;
    class Swapchain;

    class App {
    public:
        App();
        ~App();

        void Start();
    private:
        std::unique_ptr<Window> m_window;
        std::unique_ptr<Device> m_device;
        std::unique_ptr<Swapchain> m_swapchain;

        bool m_window_resized = false;

        void recreate_swapchain(RenderData &data);

        int CreateRenderPassVulkan(RenderData &data) const;
        [[nodiscard]] VkShaderModule CreateShaderModuleVulkan(const std::vector<char> &code) const;
        int CreateGraphicsPipelineVulkan(RenderData &data) const;
        int CreateFramebuffersVulkan(RenderData &data) const;
        int CreateCommandPoolVulkan(RenderData &data) const;
        int CreateCommandBuffersVulkan(RenderData &data) const;

        int DrawFrame(RenderData &data);

        int InitializeVulkan(RenderData &data);
        void InitializeImGui(const RenderData &data) const;

        void InitializeCallbacks();

        void Cleanup(const RenderData& renderData);
    };

} // Kynetic