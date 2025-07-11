//
// Created by kenny on 3-7-25.
//

#pragma once

namespace Kynetic {

class Buffer;
class Device;
class Swapchain;
class Renderer;
class CommandPool;
class ImGuiRenderer;

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }
};

class Renderer {
protected:
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    std::vector<Buffer> buffers_queued_for_destruction;

    std::unique_ptr<Device> m_device;
    std::unique_ptr<Swapchain> m_swapchain;

    std::unique_ptr<CommandPool> m_cmd_pool;
    std::vector<VkCommandBuffer> m_cmd_bufs;

    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;

    Buffer* m_vertex_buffer = nullptr;
    Buffer* m_index_buffer = nullptr;

public:
    Renderer() = default;
    ~Renderer();

    void create_render_pass();
    void create_graphics_pipeline();

    void create_vertex_buffer(VkCommandBuffer cmd_buf, std::vector<Buffer> &buffers_queued_for_destruction);
    void create_index_buffer(VkCommandBuffer cmd_buf, std::vector<Buffer> &buffers_queued_for_destruction);

    [[nodiscard]] VkRenderPass get_render_pass() const { return m_render_pass; }
    [[nodiscard]] VkPipeline get_pipeline() const { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout get_layout() const { return m_layout; }
};

} // Kynetic
