//
// Created by kennypc on 11/5/25.
//

#pragma once

namespace kynetic
{

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

    int m_frame_count{0};

    std::shared_ptr<class Shader> m_gradient;
    VkDescriptorSet m_gradient_descriptor_set{VK_NULL_HANDLE};

    std::shared_ptr<Shader> m_triangle_frag;
    std::shared_ptr<Shader> m_triangle_vert;

    std::unique_ptr<Pipeline> m_mesh_pipeline;

    std::shared_ptr<class Mesh> m_quad;

    struct Effect
    {
        const char* name;

        std::unique_ptr<Pipeline> pipeline;
        ComputePushConstants data{};
    };

    std::vector<Effect> backgroundEffects;
    int currentBackgroundEffect{0};

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