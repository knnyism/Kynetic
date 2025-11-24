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

    uint32_t m_instance_count{0};

    AllocatedBuffer m_instance_data_buffer{VK_NULL_HANDLE};
    AllocatedBuffer m_indirect_command_buffer{VK_NULL_HANDLE};

    std::vector<VkDrawIndexedIndirectCommand> m_draw_commands;

    VkDeviceAddress m_instance_data_buffer_address{0};

    bool update_instance_data_buffer(const flecs::query<struct TransformComponent, struct MeshComponent>& query);
    void update_indirect_commmand_buffer();
    void update();

public:
    Scene();
    ~Scene();

    flecs::entity add_model(const std::shared_ptr<class Model>& model) const;
    flecs::world& get() { return m_scene; }

    [[nodiscard]] const std::vector<VkDrawIndexedIndirectCommand>& get_draw_commands() const { return m_draw_commands; }
    [[nodiscard]] VkDeviceAddress get_instance_data() const { return m_instance_data_buffer_address; }
    [[nodiscard]] uint32_t get_draw_count() const { return static_cast<uint32_t>(m_draw_commands.size()); }
};

}  // namespace kynetic
