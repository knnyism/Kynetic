//
// Created by kenny on 6/30/25.
//

#pragma once
#include "Window.h"

struct RenderData {
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> availableSemaphores;
    std::vector<VkSemaphore> finishedSemaphore;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imageInFlight;
    size_t currentFrame = 0;
};

namespace Kynetic {

class App {
public:
    App();

    void Start();
private:
    std::unique_ptr<Window> m_window;

    GLFWwindow* window{};
    vkb::Instance instance;
    vkb::InstanceDispatchTable instanceDispatch;
    VkSurfaceKHR surface{};
    vkb::Device device;
    vkb::DispatchTable dispatch;
    vkb::Swapchain swapchain;

    int InitializeDeviceVulkan();
    int CreateSwapchainVulkan();
    int GetQueuesVulkan(RenderData &data) const;
    int CreateRenderPassVulkan(RenderData &data) const;
    [[nodiscard]] VkShaderModule CreateShaderModuleVulkan(const std::vector<char> &code) const;
    int CreateGraphicsPipelineVulkan(RenderData &data) const;
    int CreateFramebuffersVulkan(RenderData &data);
    int CreateCommandPoolVulkan(RenderData &data) const;
    int CreateCommandBuffersVulkan(RenderData &data) const;
    int CreateSyncObjectsVulkan(RenderData &data) const;
    int RecreateSwapchainVulkan(RenderData &data);

    int DrawFrame(RenderData &data);

    int InitializeVulkan(RenderData &data);
    void InitializeImGui(const RenderData &data) const;

    void Cleanup(const RenderData& renderData);
};

} // Kynetic
