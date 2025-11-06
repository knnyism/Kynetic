//
// Created by kennypc on 11/5/25.
//

#pragma once

namespace kynetic
{

class Shader;

class Renderer
{
    friend class Engine;

    AllocatedImage m_draw_image;
    DescriptorAllocator m_descriptor_allocator;

    DeletionQueue m_deletion_queue;

    std::shared_ptr<Shader> m_gradient_shader;

    VkDescriptorSet m_draw_image_descriptors;
    VkDescriptorSetLayout m_draw_image_descriptor_layout;
    VkPipeline m_gradient_pipeline;
    VkPipelineLayout m_gradient_pipeline_layout;

    int m_frame_count{0};

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