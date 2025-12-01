//
// Created by kenny on 11/5/25.
//

#pragma once

namespace kynetic
{

class Renderer
{
    friend class Engine;

    AllocatedImage m_render_target;
    AllocatedImage m_depth_render_target;

    DeletionQueue m_deletion_queue;

    std::shared_ptr<class Shader> m_lit_shader;
    std::shared_ptr<class Shader> m_mesh_lit_shader;

    std::unique_ptr<class Pipeline> m_lit_pipeline;
    std::unique_ptr<class Pipeline> m_mesh_lit_pipeline;

    float m_render_scale{1.f};
    RenderMode m_rendering_method{RenderMode::CpuDriven};
    RenderChannel m_render_channel{RenderChannel::Final};
    VkExtent2D m_last_device_extent;

    void init_render_target();
    void destroy_render_target() const;

    void update();
    void render();

public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;
};
}  // namespace kynetic