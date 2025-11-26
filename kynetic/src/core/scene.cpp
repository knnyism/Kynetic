//
// Created by kenny on 11/19/25.
//

#include "core/engine.hpp"
#include "core/device.hpp"

#include "core/components.hpp"
#include "rendering/material.hpp"
#include "rendering/mesh.hpp"
#include "rendering/model.hpp"

#include "scene.hpp"

using namespace kynetic;

Scene::Scene()
{
    m_root = m_scene.entity().add<TransformComponent>();

    m_scene.observer<TransformComponent>("TransformDirty")
        .event(flecs::OnSet)
        .each([](TransformComponent& transform) { transform.is_dirty = true; });
}

void Scene::update()
{
    m_scene.query_builder<TransformComponent>().each(
        [](const flecs::entity entity, const TransformComponent& transform)
        {
            if (transform.is_dirty)
                entity.children<TransformComponent>(
                    [](const flecs::entity child)
                    {
                        if (auto* child_transform = child.try_get_mut<TransformComponent>()) child_transform->is_dirty = true;
                    });
        });

    m_scene.query_builder<TransformComponent, TransformComponent>().term_at(1).parent().cascade().build().each(
        [](TransformComponent& transform, const TransformComponent& transform_parent)
        {
            if (transform.is_dirty)
            {
                transform.is_dirty = false;
                transform.transform = transform_parent.transform *
                                      make_transform_matrix(transform.translation, transform.rotation, transform.scale);
            }
        });

    const Device& device = Engine::get().device();
    const float aspect = static_cast<float>(device.get_extent().width) / static_cast<float>(device.get_extent().height);

    m_scene.query_builder<CameraComponent, TransformComponent, MainCameraTag>().build().each(
        [&](const CameraComponent& camera, const TransformComponent& transform, const MainCameraTag&)
        {
            m_view = glm::inverse(transform.transform);
            m_projection = glm::perspective(camera.fovy, aspect * camera.aspect, camera.far_plane, camera.near_plane);
            m_projection[1][1] *= -1;
        });

    update_instance_data_buffer();
    update_indirect_commmand_buffer();
}

bool Scene::update_instance_data_buffer()
{
    Device& device = Engine::get().device();

    std::unordered_map<Mesh*, std::vector<InstanceData>> instances_by_mesh;

    uint32_t instance_count = 0;

    m_scene.query_builder<TransformComponent, MeshComponent>().build().each(
        [&](const TransformComponent& transform, const MeshComponent& mesh_component)
        {
            for (const auto& mesh : mesh_component.meshes)
            {
                instances_by_mesh[mesh.get()].push_back({transform.transform,
                                                         glm::transpose(glm::inverse(transform.transform)),
                                                         mesh->get_material()->get_handle()});
                instance_count++;
            }
        });

    std::vector<InstanceData> instances{instance_count};
    size_t instance_data_size = instance_count * sizeof(InstanceData);

    const bool should_rebuild_draw_commands = m_instance_count != instance_count;
    m_instance_count = instance_count;

    uint32_t instance_index = 0;
    if (should_rebuild_draw_commands)
    {
        m_draw_commands.clear();
        for (auto [mesh, instance_data] : instances_by_mesh)
        {
            VkDrawIndexedIndirectCommand& batch = m_draw_commands.emplace_back();
            batch.firstIndex = mesh->get_index_offset();
            batch.indexCount = mesh->get_index_count();
            batch.firstInstance = instance_index;
            batch.instanceCount = static_cast<uint32_t>(instance_data.size());
            batch.vertexOffset = static_cast<int32_t>(mesh->get_vertex_offset());

            for (auto& data : instance_data)
            {
                instances[instance_index].model = data.model;
                instances[instance_index].model_inv = data.model_inv;
                instances[instance_index].material_index = data.material_index;
                instance_index++;
            }
        }
    }
    else
    {
        for (auto [mesh, instance_data] : instances_by_mesh)
        {
            for (auto& data : instance_data)
            {
                instances[instance_index].model = data.model;
                instances[instance_index].model_inv = data.model_inv;
                instances[instance_index].material_index = data.material_index;
                instance_index++;
            }
        }
    }

    uint32_t frame_index = device.get_frame_index();
    Context& ctx = device.get_context();

    auto& instance_data_buffer = m_instance_data_buffers[frame_index];

    instance_data_buffer = device.create_buffer(
        instance_data_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(instance_data_buffer); });

    const AllocatedBuffer staging =
        device.create_buffer(instance_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(staging); });

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);
    memcpy(data, instances.data(), instance_data_size);
    vmaUnmapMemory(device.get_allocator(), staging.allocation);

    VkBufferCopy copy{};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = instance_data_size;

    ctx.dcb.copy_buffer(staging.buffer, instance_data_buffer.buffer, 1, &copy);

    return should_rebuild_draw_commands;
}

void Scene::update_indirect_commmand_buffer()
{
    Device& device = Engine::get().device();

    uint32_t frame_index = device.get_frame_index();
    Context& ctx = device.get_context();

    auto& indirect_command_buffer = m_indirect_command_buffer[frame_index];

    size_t indirect_buffer_size = sizeof(VkDrawIndexedIndirectCommand) * m_draw_commands.size();

    indirect_command_buffer = device.create_buffer(indirect_buffer_size,
                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                       VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                       VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                   VMA_MEMORY_USAGE_GPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(indirect_command_buffer); });

    AllocatedBuffer staging =
        device.create_buffer(indirect_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    ctx.deletion_queue.push_function([=, &device] { device.destroy_buffer(staging); });

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);
    memcpy(data, m_draw_commands.data(), indirect_buffer_size);
    vmaUnmapMemory(device.get_allocator(), staging.allocation);

    VkBufferCopy copy{};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = indirect_buffer_size;

    ctx.dcb.copy_buffer(staging.buffer, indirect_command_buffer.buffer, 1, &copy);
}

flecs::entity Scene::add_camera(bool is_main_camera) const
{
    const flecs::entity camera_entity =
        m_scene.entity()
            .add<TransformComponent>()
            .set<CameraComponent>({.aspect = 1.f, .near_plane = 0.1f, .far_plane = 1000.f, .fovy = 70.f})
            .child_of(m_root);

    if (is_main_camera) camera_entity.add<MainCameraTag>();

    return camera_entity;
}

flecs::entity Scene::add_model(const std::shared_ptr<Model>& model) const
{
    const flecs::entity root_entity = m_scene.entity().child_of(m_root);

    std::function<void(const Model::Node&, flecs::entity)> traverse_nodes =
        [&](const Model::Node& node, const flecs::entity parent)
    {
        const flecs::entity child_entity = m_scene.entity().child_of(parent).add<TransformComponent>();

        if (!node.meshes.empty())
        {
            child_entity.add<MeshComponent>();
            child_entity.set<MeshComponent>({node.meshes});
        }

        child_entity.set<TransformComponent>({.translation = node.translation, .rotation = node.rotation, .scale = node.scale});

        for (const auto& child : node.children) traverse_nodes(child, child_entity);
    };

    traverse_nodes(model->get_root_node(), root_entity);

    return root_entity;
}

VkDeviceAddress Scene::get_instance_data_buffer_address() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    const VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                        .buffer = m_instance_data_buffers[frame_index].buffer};
    return vkGetBufferDeviceAddress(device.get(), &device_address_info);
}

VkBuffer Scene::get_indirect_commmand_buffer() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    return m_indirect_command_buffer[frame_index].buffer;
}

VkDeviceAddress Scene::get_indirect_commmand_buffer_address() const
{
    const Device& device = Engine::get().device();
    uint32_t frame_index = device.get_frame_index();

    const VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                        .buffer = m_indirect_command_buffer[frame_index].buffer};
    return vkGetBufferDeviceAddress(device.get(), &device_address_info);
}
