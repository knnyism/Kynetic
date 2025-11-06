//
// Created by kennypc on 11/5/25.
//

#pragma once

namespace kynetic
{

class Shader;

struct ComputePushConstants
{
    glm::vec4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
};

class Renderer
{
    friend class Engine;

    AllocatedImage m_draw_image;

    DeletionQueue m_deletion_queue;

    std::shared_ptr<Shader> m_gradient;
    VkDescriptorSet m_draw_image_descriptor_set;
    VkPipeline m_gradient_pipeline;

    int m_frame_count{0};
    ComputePushConstants data;

public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void render();
};
}  // namespace kynetic