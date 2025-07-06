//
// Created by kenny on 3-7-25.
//

#pragma once

namespace Kynetic {

class Device;
class Swapchain;
class Renderer;
class CommandPool;
class ImGuiRenderer;

class Renderer {
protected:
    std::unique_ptr<Device> m_device;
    std::unique_ptr<Swapchain> m_swapchain;

    std::unique_ptr<CommandPool> m_cmd_pool;
    std::vector<VkCommandBuffer> m_cmd_bufs;

    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
public:
    Renderer() = default;
    ~Renderer();

    void create_render_pass();
    void create_graphics_pipeline();

    [[nodiscard]] VkRenderPass get_render_pass() const { return m_render_pass; }
    [[nodiscard]] VkPipeline get_pipeline() const { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout get_layout() const { return m_layout; }
};

} // Kynetic
