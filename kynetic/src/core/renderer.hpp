//
// Created by kenny on 11/5/25.
//

#pragma once

namespace kynetic
{

class Shader;

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

    std::shared_ptr<Shader> m_lit_shader;
    std::shared_ptr<Shader> m_frustum_cull_shader;

    std::unique_ptr<Pipeline> m_lit_pipeline;
    std::unique_ptr<Pipeline> m_frustum_cull_pipeline;

    float m_render_scale{1.f};
    RenderingMethod m_rendering_method{RenderingMethod::CpuDriven};
    RenderChannel m_render_channel{RenderChannel::Final};
    VkExtent2D m_last_device_extent;

    void init_render_target();
    void destroy_render_target() const;
    void gpu_frustum_cull() const;
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