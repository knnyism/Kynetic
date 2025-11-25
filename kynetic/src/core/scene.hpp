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

    AllocatedBuffer m_instance_data_buffer{VK_NULL_HANDLE};
    AllocatedBuffer m_indirect_command_buffer{VK_NULL_HANDLE};

    std::vector<VkDrawIndexedIndirectCommand> m_draw_commands;

    VkDeviceAddress m_instance_data_buffer_address{0};

    glm::mat4 m_projection{1.f};
    glm::mat4 m_view{1.f};

    bool update_instance_data_buffer();
    void update_indirect_commmand_buffer();
    void update();

public:
    bool m_paused{false};
     Scene();
    ~Scene();

    flecs::entity add_camera(bool is_main_camera) const;
    flecs::entity add_model(const std::shared_ptr<class Model>& model) const;
    flecs::world& get() { return m_scene; }

    [[nodiscard]] const std::vector<VkDrawIndexedIndirectCommand>& get_draw_commands() const { return m_draw_commands; }
    [[nodiscard]] VkDeviceAddress get_instance_data_buffer_address() const { return m_instance_data_buffer_address; }
    [[nodiscard]] uint32_t get_draw_count() const { return static_cast<uint32_t>(m_draw_commands.size()); }

    [[nodiscard]] glm::mat4 get_projection() const { return m_projection; }
    [[nodiscard]] glm::mat4 get_view() const { return m_view; }
};

}  // namespace kynetic
