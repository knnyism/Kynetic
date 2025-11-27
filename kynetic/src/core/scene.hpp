//
// Created by kenny on 11/19/25.
//

#pragma once

namespace kynetic
{

class Scene
{
    friend class Engine;
    friend class Renderer;

    flecs::world m_scene;
    flecs::entity m_root;

    uint32_t m_instance_count{0};
    size_t m_instance_data_size{0};

    AllocatedBuffer m_instance_data_buffers[4]{VK_NULL_HANDLE};
    AllocatedBuffer m_indirect_command_buffer[4]{VK_NULL_HANDLE};

    std::vector<InstanceData> m_instances;
    std::vector<VkDrawIndexedIndirectCommand> m_draw_commands;

    glm::mat4 m_projection{1.f};
    glm::mat4 m_projection_debug{1.f};
    glm::mat4 m_view{1.f};

    void update();

public:
    Scene();
    ~Scene() = default;

    flecs::entity add_camera(bool is_main_camera) const;
    flecs::entity add_model(const std::shared_ptr<class Model>& model) const;
    flecs::world& get() { return m_scene; }

    [[nodiscard]] VkDeviceAddress get_instance_data_buffer_address() const;
    [[nodiscard]] VkBuffer get_instance_data_buffer() const;
    [[nodiscard]] size_t get_instance_data_buffer_size() const { return m_instance_data_size; }

    [[nodiscard]] std::vector<InstanceData>& get_instances() { return m_instances; }
    [[nodiscard]] std::vector<VkDrawIndexedIndirectCommand>& get_draw_commands() { return m_draw_commands; }

    [[nodiscard]] VkDeviceAddress get_indirect_commmand_buffer_address() const;
    [[nodiscard]] VkBuffer get_indirect_commmand_buffer() const;
    [[nodiscard]] uint32_t get_draw_count() const { return static_cast<uint32_t>(m_draw_commands.size()); }

    [[nodiscard]] glm::mat4 get_projection() const { return m_projection; }
    [[nodiscard]] glm::mat4 get_projection_debug() const { return m_projection_debug; }
    [[nodiscard]] glm::mat4 get_view() const { return m_view; }

    bool update_instance_data_buffer();
    void update_indirect_commmand_buffer();
};

}  // namespace kynetic
