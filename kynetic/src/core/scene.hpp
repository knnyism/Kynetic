//
// Created by kenny on 11/19/25.
//

#pragma once

namespace kynetic
{

struct MeshletDrawInfo
{
    std::shared_ptr<class Mesh> mesh;
    uint32_t instance_index;
};

class Scene
{
    friend class Engine;
    friend class Renderer;

    flecs::world m_scene;
    flecs::entity m_root;

    std::shared_ptr<class Shader> m_cull_shader;
    std::unique_ptr<class Pipeline> m_cull_pipeline;

    std::vector<InstanceData> m_instances;
    AllocatedBuffer m_instances_buffers[MAX_FRAMES_IN_FLIGHT]{VK_NULL_HANDLE};
    VkDeviceAddress m_instances_buffer_address{0};

    std::vector<VkDrawIndexedIndirectCommand> m_draws;
    AllocatedBuffer m_draw_buffers[MAX_FRAMES_IN_FLIGHT]{VK_NULL_HANDLE};
    VkDeviceAddress m_draw_buffer_address{0};

    std::vector<MeshDrawCommand> m_mesh_draws;

    SceneData m_scene_data;
    AllocatedBuffer m_scene_buffers[MAX_FRAMES_IN_FLIGHT]{VK_NULL_HANDLE};
    VkDeviceAddress m_scene_buffer_address{0};

    glm::mat4 m_projection{1.f};
    glm::mat4 m_view{1.f};

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

    [[nodiscard]] const std::vector<MeshletDrawInfo>& get_meshlet_draws() const { return m_meshlet_draws; }

    [[nodiscard]] VkDeviceAddress get_instance_buffer_address() const { return m_instances_buffer_address; }
    [[nodiscard]] VkDeviceAddress get_scene_buffer_address() const { return m_scene_buffer_address; }

    [[nodiscard]] AllocatedBuffer get_instance_buffer() const;
    [[nodiscard]] AllocatedBuffer get_draw_buffer() const;
    [[nodiscard]] AllocatedBuffer get_scene_buffer() const;

    void cull(RenderMode render_mode);
    void draw(RenderMode render_mode) const;
};

}  // namespace kynetic
