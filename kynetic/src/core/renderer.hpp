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

    std::shared_ptr<class Shader> m_clear_shader;
    std::shared_ptr<class Shader> m_lit_shader;
    std::shared_ptr<class Shader> m_mesh_lit_shader;

    std::unique_ptr<class Pipeline> m_clear_pipeline;
    std::unique_ptr<class Pipeline> m_lit_pipeline;
    std::unique_ptr<class Pipeline> m_mesh_lit_pipeline;

    std::shared_ptr<class Shader> m_debug_line_shader;
    std::shared_ptr<class Shader> m_debug_meshlet_shader;

    std::unique_ptr<class Pipeline> m_debug_line_pipeline;
    std::unique_ptr<class Pipeline> m_debug_meshlet_pipeline;

    AllocatedBuffer m_stats_buffer;
    AllocatedBuffer m_stats_readback_buffer;
    VkDeviceAddress m_stats_buffer_address{0};

    AllocatedBuffer m_meshlet_debug_buffers[MAX_FRAMES_IN_FLIGHT];
    uint32_t m_debug_meshlet_count{0};
    static constexpr uint32_t MAX_DEBUG_MESHLETS = 65536;

    float m_render_scale{1.f};
    RenderMode m_rendering_method{RenderMode::CpuDriven};
    RenderChannel m_render_channel{RenderChannel::Final};
    VkExtent2D m_last_device_extent;

    void init_render_target();
    void destroy_render_target() const;

    void init_debug_resources();
    void destroy_debug_resources();

    void render_debug_visualizations();
    void render_frustum_lines();
    void render_meshlet_debug();
    void readback_stats();

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