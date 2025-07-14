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

    static VkVertexInputBindingDescription get_binding_description() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class Renderer {
protected:
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

    std::vector<Buffer> buffers_queued_for_destruction;

    std::unique_ptr<Device> m_device;
    std::unique_ptr<Swapchain> m_swapchain;

    std::unique_ptr<CommandPool> m_cmd_pool;
    std::vector<VkCommandBuffer> m_cmd_bufs;

    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptor_pool;
    std::vector<VkDescriptorSet> m_descriptor_sets;

    Buffer* m_vertex_buffer = nullptr;
    Buffer* m_index_buffer = nullptr;
    std::vector<Buffer> m_uniform_buffers;
public:
    Renderer() = default;
    ~Renderer();

    void create_render_pass();
    void create_graphics_pipeline();
    void create_descriptor_set_layout();

    void create_descriptor_pool();

    void create_descriptor_sets();

    void create_uniform_buffers();

    void update_ubo(uint32_t image_index);

    void create_vertex_buffer(VkCommandBuffer cmd_buf, std::vector<Buffer> &buffers_queued_for_destruction);
    void create_index_buffer(VkCommandBuffer cmd_buf, std::vector<Buffer> &buffers_queued_for_destruction);

    [[nodiscard]] VkRenderPass get_render_pass() const { return m_render_pass; }
    [[nodiscard]] VkPipeline get_pipeline() const { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout get_layout() const { return m_layout; }
};

} // Kynetic
