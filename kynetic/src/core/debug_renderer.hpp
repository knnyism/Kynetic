//
// Debug Renderer - Visualization tools for debugging culling and meshlets
//

#pragma once

namespace kynetic
{

struct DebugSettings
{
    bool show_camera_frustum{false};
    bool show_meshlet_spheres{false};
    bool show_meshlet_cones{false};
    bool pause_culling{false};

    float sphere_opacity{0.5f};
    float cone_opacity{0.5f};
    float frustum_opacity{0.3f};

    // Stats
    uint32_t active_meshlet_count{0};
    uint32_t total_meshlet_count{0};
    uint32_t active_instance_count{0};
    uint32_t total_instance_count{0};
};

struct DebugVertex
{
    glm::vec3 position;
    glm::vec4 color;
};

// CPU-side copy of meshlet bounds for debug visualization
struct DebugMeshletBounds
{
    glm::vec3 center;
    float radius;
    glm::vec3 cone_axis;
    float cone_cutoff;
};

class DebugRenderer
{
    friend class Engine;
    friend class Renderer;

    DebugSettings m_settings;

    // Cached frustum VP for pause culling
    glm::mat4 m_cached_vp{1.0f};
    bool m_has_cached_vp{false};

    // Line rendering
    std::vector<DebugVertex> m_line_vertices;
    AllocatedBuffer m_line_vertex_buffer{};
    VkDeviceAddress m_line_vertex_buffer_address{0};

    // Triangle rendering (for solid shapes)
    std::vector<DebugVertex> m_triangle_vertices;
    AllocatedBuffer m_triangle_vertex_buffer{};
    VkDeviceAddress m_triangle_vertex_buffer_address{0};

    std::shared_ptr<class Shader> m_debug_line_shader;
    std::shared_ptr<class Shader> m_debug_solid_shader;
    std::unique_ptr<class Pipeline> m_debug_line_pipeline;
    std::unique_ptr<class Pipeline> m_debug_solid_pipeline;

    bool m_buffers_dirty{true};

    void generate_sphere_mesh(const glm::vec3& center, float radius, const glm::vec4& color, int segments = 12, int rings = 8);
    void generate_cone_mesh(const glm::vec3& apex,
                            const glm::vec3& axis,
                            float height,
                            float angle,
                            const glm::vec4& color,
                            int segments = 12);
    void generate_frustum_lines(const glm::mat4& inv_view_proj, const glm::vec4& color);

    void update_buffers();
    void render(const VkExtent2D& extent);
    void render_ui();

public:
    DebugRenderer();
    ~DebugRenderer();

    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer(DebugRenderer&&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;
    DebugRenderer& operator=(DebugRenderer&&) = delete;

    void clear();

    void add_line(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color);
    void add_sphere(const glm::vec3& center, float radius, const glm::vec4& color);
    void add_cone(const glm::vec3& apex, const glm::vec3& axis, float height, float angle, const glm::vec4& color);
    void add_frustum(const glm::mat4& view_proj, const glm::vec4& color);

    [[nodiscard]] DebugSettings& settings() { return m_settings; }
    [[nodiscard]] const DebugSettings& settings() const { return m_settings; }

    [[nodiscard]] bool is_culling_paused() const { return m_settings.pause_culling; }

    // For pause culling - cache and retrieve VP matrix
    void cache_vp(const glm::mat4& vp)
    {
        m_cached_vp = vp;
        m_has_cached_vp = true;
    }
    [[nodiscard]] const glm::mat4& get_cached_vp() const { return m_cached_vp; }
    [[nodiscard]] bool has_cached_vp() const { return m_has_cached_vp; }
};

}  // namespace kynetic