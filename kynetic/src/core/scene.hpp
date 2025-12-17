//
// Created by kenny on 11/19/25.
//

#pragma once

namespace kynetic
{
struct TransformComponent;
struct MeshComponent;
struct CameraComponent;
struct MainCameraTag;

struct DebugSettings
{
    bool pause_culling{false};
    bool show_frustum{false};
    bool show_meshlet_spheres{false};
    bool show_meshlet_cones{false};

    glm::mat4 frozen_view{1.f};
    glm::mat4 frozen_projection{1.f};
    glm::mat4 frozen_vp{1.f};
    glm::vec3 frozen_camera_position{0.f};

    uint32_t total_meshlets{0};
    uint32_t visible_meshlets{0};
};

class Scene
{
    friend class Engine;
    friend class Renderer;

    flecs::world m_scene;
    flecs::entity m_root;

    flecs::query<TransformComponent> m_transform_dirty_query;
    flecs::query<TransformComponent, TransformComponent> m_transform_hierarchy_query;
    flecs::query<CameraComponent, TransformComponent, MainCameraTag> m_camera_query;
    flecs::query<TransformComponent, MeshComponent> m_mesh_query;

    std::shared_ptr<class Shader> m_cull_shader;
    std::unique_ptr<class Pipeline> m_cull_pipeline;

    std::vector<InstanceData> m_instances;
    AllocatedBuffer m_instances_buffers[MAX_FRAMES_IN_FLIGHT]{VK_NULL_HANDLE};
    VkDeviceAddress m_instances_buffer_address{0};

    std::vector<VkDrawIndexedIndirectCommand> m_draws;
    AllocatedBuffer m_draw_buffers[MAX_FRAMES_IN_FLIGHT]{VK_NULL_HANDLE};
    VkDeviceAddress m_draw_buffer_address{0};

    std::vector<MeshDrawData> m_mesh_draw_data;
    AllocatedBuffer m_mesh_draw_data_buffers[MAX_FRAMES_IN_FLIGHT]{VK_NULL_HANDLE};
    VkDeviceAddress m_mesh_draw_data_buffer_address{0};

    std::vector<VkDrawMeshTasksIndirectCommandEXT> m_mesh_indirect_commands;
    AllocatedBuffer m_mesh_indirect_buffers[MAX_FRAMES_IN_FLIGHT]{VK_NULL_HANDLE};

    SceneData m_scene_data;
    AllocatedBuffer m_scene_buffers[MAX_FRAMES_IN_FLIGHT]{VK_NULL_HANDLE};
    VkDeviceAddress m_scene_buffer_address{0};

    glm::mat4 m_projection{1.f};
    glm::mat4 m_view{1.f};
    glm::mat4 m_previous_vp = glm::mat4(1.0f);

    DebugSettings m_debug_settings;

    void cpu_cull(const glm::mat4& vp);
    void gpu_cull() const;

    void update();
    void update_buffers();

public:
    Scene();
    ~Scene();

    [[nodiscard]] flecs::world& get() { return m_scene; }

    flecs::entity add_camera(bool is_main_camera) const;
    flecs::entity add_model(const std::shared_ptr<class Model>& model) const;

    [[nodiscard]] glm::mat4 get_projection() const { return m_projection; }
    [[nodiscard]] glm::mat4 get_view() const { return m_view; }

    [[nodiscard]] const std::vector<VkDrawIndexedIndirectCommand>& get_draws() const { return m_draws; }
    [[nodiscard]] uint32_t get_draw_count() const { return static_cast<uint32_t>(m_draws.size()); }

    [[nodiscard]] uint32_t get_mesh_draw_count() const { return static_cast<uint32_t>(m_mesh_draw_data.size()); }

    [[nodiscard]] VkDeviceAddress get_instance_buffer_address() const { return m_instances_buffer_address; }
    [[nodiscard]] VkDeviceAddress get_scene_buffer_address() const { return m_scene_buffer_address; }
    [[nodiscard]] VkDeviceAddress get_mesh_draw_data_buffer_address() const { return m_mesh_draw_data_buffer_address; }

    [[nodiscard]] AllocatedBuffer get_instance_buffer() const;
    [[nodiscard]] AllocatedBuffer get_draw_buffer() const;
    [[nodiscard]] AllocatedBuffer get_scene_buffer() const;
    [[nodiscard]] AllocatedBuffer get_mesh_indirect_buffer() const;

    [[nodiscard]] DebugSettings& get_debug_settings() { return m_debug_settings; }
    [[nodiscard]] const DebugSettings& get_debug_settings() const { return m_debug_settings; }

    void freeze_culling_camera();
    void unfreeze_culling_camera();

    std::vector<glm::vec3> get_frustum_corners(const glm::mat4& vp) const;
    std::vector<DebugLineVertex> get_frustum_lines() const;

    void cull(RenderMode render_mode);
    void draw(RenderMode render_mode) const;
};
}  // namespace kynetic