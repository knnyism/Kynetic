//
// Created by kenny on 6/30/25.
//

#pragma once

struct RenderData {
    VkQueue graphics_queue;
    VkQueue present_queue;

    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    std::vector<VkSemaphore> available_semaphores;
    std::vector<VkSemaphore> finished_semaphore;
    std::vector<VkFence> in_flight_fences;
    std::vector<VkFence> image_in_flight;
    size_t current_frame = 0;
};

namespace Kynetic {
    class App {
    public:
        void Start();
    private:
        GLFWwindow* window;
        vkb::Instance instance;
        vkb::InstanceDispatchTable inst_disp;
        VkSurfaceKHR surface;
        vkb::Device device;
        vkb::DispatchTable disp;
        vkb::Swapchain swapchain;

        GLFWwindow *CreateWindowGlfw();
        void DestroyWindowGlfw() const;
        [[nodiscard]] VkSurfaceKHR CreateSurfaceGlfw(VkInstance instance, GLFWwindow *window, VkAllocationCallbacks *allocator) const;

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

        void InitializeImGui();

        void Cleanup(RenderData& renderData);
    };
}
