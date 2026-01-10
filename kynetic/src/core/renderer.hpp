//
// Created by kenny on 11/5/25.
//

#pragma once

#include "rendering/descriptor.hpp"

namespace kynetic
{

struct PerformanceStats
{
    float frame_time_ms{0.0f};
    float fps{0.0f};
    float avg_frame_time_ms{0.0f};
    float min_frame_time_ms{FLT_MAX};
    float max_frame_time_ms{0.0f};

    uint32_t total_triangles{0};
    uint32_t rendered_triangles{0};

    uint64_t task_shader_invocations{0};
    uint64_t mesh_shader_invocations{0};
    uint64_t mesh_shader_primitives{0};

    static constexpr size_t FRAME_HISTORY_SIZE = 120;
    float frame_time_history[FRAME_HISTORY_SIZE]{};
    size_t frame_time_index{0};
    size_t frame_time_count{0};
};

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
    RenderChannel m_render_channel{RenderChannel::Final};
    DebugMeshletRenderMode m_meshlet_render_mode{DebugMeshletRenderMode::Cluster};

    VkExtent2D m_last_device_extent;
    float m_last_render_scale{1.f};

    PerformanceStats m_perf_stats;
    std::chrono::high_resolution_clock::time_point m_last_frame_time;
    bool m_show_perf_overlay{true};
    bool m_enable_shader_stats{false};

    VkQueryPool m_pipeline_stats_query_pool{VK_NULL_HANDLE};
    static constexpr uint32_t QUERY_COUNT = MAX_FRAMES_IN_FLIGHT;
    bool m_query_results_available[MAX_FRAMES_IN_FLIGHT]{};

    void init_render_target();
    void destroy_render_target() const;

    void init_depth_pyramid();
    void destroy_depth_pyramid() const;

    void init_query_pools();
    void destroy_query_pools();

    void render_debug_visualizations();
    void render_frustum_lines();

    void render_performance_overlay();
    void update_frametime_stats(float delta_time_ms);

    void update();
    void render();

public:
    Renderer();
    ~Renderer();

    void render_imgui();

    [[nodiscard]] const PerformanceStats& get_perf_stats() const { return m_perf_stats; }

    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;
};
}  // namespace kynetic