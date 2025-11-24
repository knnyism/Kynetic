//
// Created by kenny on 11/5/25.
//

#pragma once

namespace kynetic
{
class Renderer
{
    enum class RenderingMethod
    {
        CpuDriven,
        GpuDriven,

        GpuDrivenMeshlets
    };

    friend class Engine;

    AllocatedImage m_render_target;
    AllocatedImage m_depth_render_target;

    DeletionQueue m_deletion_queue;

    int m_frame_count{0};

    std::shared_ptr<class Shader> m_gradient;

    std::shared_ptr<Shader> m_lit_shader;

    std::shared_ptr<Shader> m_triangle_frag;
    std::shared_ptr<Shader> m_triangle_vert;

    std::unique_ptr<Pipeline> m_lit_pipeline;

    std::shared_ptr<class Texture> m_texture;

    float m_render_scale{1.f};
    RenderingMethod m_rendering_method{RenderingMethod::CpuDriven};

    VkExtent2D m_last_device_extent;

    struct Effect
    {
        const char* name;

        std::unique_ptr<Pipeline> pipeline;
        GradientPushConstants data{};
    };

    std::vector<Effect> backgroundEffects;
    int currentBackgroundEffect{0};

    void init_render_target();
    void destroy_render_target() const;
    void update();

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