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

    std::unique_ptr<class Pipeline> m_clear_pipeline;
    std::unique_ptr<class Pipeline> m_lit_pipeline;
    std::unique_ptr<class Pipeline> m_mesh_lit_pipeline;

    std::unique_ptr<class Pipeline> m_debug_line_pipeline;

    std::unique_ptr<class Pipeline> m_depth_pyramid_pipeline;

    uint32_t m_depth_pyramid_levels;

    AllocatedImage m_depth_pyramid;
    DescriptorAllocator m_depth_pyramid_allocator;
    VkSampler m_depth_pyramid_sampler;
    VkImageView m_depth_pyramid_views[MAX_DEPTH_PYRAMID_LEVELS];
    VkDescriptorSet m_depth_pyramid_sets[MAX_DEPTH_PYRAMID_LEVELS];

    float m_render_scale{1.f};
    RenderMode m_rendering_method{RenderMode::CpuDriven};
    RenderChannel m_render_channel{RenderChannel::Final};

    VkExtent2D m_last_device_extent;
    float m_last_render_scale{1.f};

    void init_render_target();
    void destroy_render_target() const;

    void init_depth_pyramid();
    void destroy_depth_pyramid() const;

    void render_debug_visualizations();
    void render_frustum_lines();

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