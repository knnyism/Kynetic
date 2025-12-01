//
// Created by kenny on 11/13/25.
//

#include "mesh.hpp"

#include <utility>

#include "core/device.hpp"
#include "core/engine.hpp"

#include "vma_usage.hpp"
#include <glm/gtx/norm.hpp>

#include "meshoptimizer.h"

using namespace kynetic;

Mesh::Mesh(const std::filesystem::path& path,
           uint32_t mesh_index,
           std::span<uint32_t> indices,
           std::span<glm::vec4> positions,
           std::span<Vertex> vertices,
           std::shared_ptr<Material> material)
    : Resource(Type::Mesh, path.string()),
      m_mesh_index(mesh_index),
      m_index_count(static_cast<uint32_t>(indices.size())),
      m_vertex_count(static_cast<uint32_t>(vertices.size())),
      m_material(std::move(material))
{
    calculate_bounds(positions);

    const size_t max_vertices = 64;
    const size_t max_triangles = 124;
    const float cone_weight = 0.0f;

    const size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);

    std::vector<meshopt_Meshlet> meshlets;
    std::vector<uint32_t> meshlet_vertex_indices;
    std::vector<uint8_t> meshlet_triangles;

    meshlets.resize(max_meshlets);

    meshlet_vertex_indices.resize(max_meshlets * max_vertices);
    meshlet_triangles.resize(max_meshlets * max_triangles * 3);

    m_meshlet_count = meshopt_buildMeshlets(meshlets.data(),
                                                 meshlet_vertex_indices.data(),
                                                 meshlet_triangles.data(),
                                                 indices.data(),
                                                 indices.size(),
                                                 reinterpret_cast<const float*>(positions.data()),
                                                 positions.size(),
                                                 sizeof(glm::vec4),
                                                 max_vertices,
                                                 max_triangles,
                                                 cone_weight);

    std::vector<MeshletData> meshlets_data;

    for (auto& meshlet : meshlets)
    {
        meshopt_Bounds meshlet_bounds = meshopt_computeMeshletBounds(meshlet_vertex_indices.data() + meshlet.vertex_offset,
                                                                     meshlet_triangles.data() + meshlet.triangle_offset,
                                                                     meshlet.triangle_count,
                                                                     reinterpret_cast<const float*>(positions.data()),
                                                                     positions.size(),
                                                                     sizeof(glm::vec4));

        MeshletData& meshlet_data = meshlets_data.emplace_back();

        meshlet_data.center = glm::vec3(meshlet_bounds.center[0], meshlet_bounds.center[1], meshlet_bounds.center[2]);
        meshlet_data.radius = meshlet_bounds.radius;

        meshlet_data.cone_axis[0] = meshlet_bounds.cone_axis_s8[0];
        meshlet_data.cone_axis[1] = meshlet_bounds.cone_axis_s8[1];
        meshlet_data.cone_axis[2] = meshlet_bounds.cone_axis_s8[2];

        meshlet_data.cone_cutoff = meshlet_bounds.cone_cutoff_s8;
        
        meshlet_data.vertex_offset = meshlet.vertex_offset;
        meshlet_data.triangle_offset = meshlet.triangle_offset;
        meshlet_data.vertex_count = static_cast<uint8_t>(meshlet.vertex_count);
        meshlet_data.triangle_count = static_cast<uint8_t>(meshlet.triangle_count);
    }

    Device& device = Engine::get().device();

    const size_t index_buffer_size = indices.size_bytes();
    const size_t position_buffer_size = positions.size_bytes();
    const size_t vertex_buffer_size = vertices.size_bytes();

    const size_t meshlet_buffer_size = meshlets_data.size() * sizeof(MeshletData);
    const size_t meshlet_vertices_buffer_size = meshlet_vertex_indices.size() * sizeof(uint32_t);
    const size_t meshlet_triangles_buffer_size = meshlet_triangles.size() * sizeof(uint8_t);

    m_index_buffer = device.create_buffer(
        index_buffer_size,
                                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_index_buffer.buffer};
        m_index_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    m_position_buffer = device.create_buffer(position_buffer_size,
                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                             VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_position_buffer.buffer};
        m_position_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    m_vertex_buffer = device.create_buffer(vertex_buffer_size,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_vertex_buffer.buffer};
        m_vertex_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    m_meshlet_buffer = device.create_buffer(meshlet_buffer_size,
                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                             VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_meshlet_buffer.buffer};
        m_meshlet_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    m_meshlet_vertices_buffer =
        device.create_buffer(meshlet_vertices_buffer_size,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                             VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_meshlet_vertices_buffer.buffer};
        m_meshlet_vertices_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    m_meshlet_triangles_buffer =
        device.create_buffer(meshlet_triangles_buffer_size,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                             VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_meshlet_triangles_buffer.buffer};
        m_meshlet_triangles_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    const size_t total_staging_size = index_buffer_size + position_buffer_size + vertex_buffer_size + meshlet_buffer_size +
                                      meshlet_vertices_buffer_size + meshlet_triangles_buffer_size;

    AllocatedBuffer staging =
        device.create_buffer(total_staging_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);

    size_t offset = 0;
    memcpy(static_cast<char*>(data) + offset, indices.data(), index_buffer_size);
    offset += index_buffer_size;

    memcpy(static_cast<char*>(data) + offset, positions.data(), position_buffer_size);
    offset += position_buffer_size;

    memcpy(static_cast<char*>(data) + offset, vertices.data(), vertex_buffer_size);
    offset += vertex_buffer_size;

    memcpy(static_cast<char*>(data) + offset, meshlets_data.data(), meshlet_buffer_size);
    offset += meshlet_buffer_size;

    memcpy(static_cast<char*>(data) + offset, meshlet_vertex_indices.data(), meshlet_vertices_buffer_size);
    offset += meshlet_vertices_buffer_size;

    memcpy(static_cast<char*>(data) + offset, meshlet_triangles.data(), meshlet_triangles_buffer_size);

    vmaUnmapMemory(device.get_allocator(), staging.allocation);

    device.immediate_submit(
        [&](const CommandBuffer& cmd)
        {
            size_t src_offset = 0;

            VkBufferCopy index_copy{};
            index_copy.dstOffset = 0;
            index_copy.srcOffset = src_offset;
            index_copy.size = index_buffer_size;
            cmd.copy_buffer(staging.buffer, m_index_buffer.buffer, 1, &index_copy);
            src_offset += index_buffer_size;

            VkBufferCopy position_copy{};
            position_copy.dstOffset = 0;
            position_copy.srcOffset = src_offset;
            position_copy.size = position_buffer_size;
            cmd.copy_buffer(staging.buffer, m_position_buffer.buffer, 1, &position_copy);
            src_offset += position_buffer_size;

            VkBufferCopy vertex_copy{};
            vertex_copy.dstOffset = 0;
            vertex_copy.srcOffset = src_offset;
            vertex_copy.size = vertex_buffer_size;
            cmd.copy_buffer(staging.buffer, m_vertex_buffer.buffer, 1, &vertex_copy);
            src_offset += vertex_buffer_size;

            VkBufferCopy meshlet_copy{};
            meshlet_copy.dstOffset = 0;
            meshlet_copy.srcOffset = src_offset;
            meshlet_copy.size = meshlet_buffer_size;
            cmd.copy_buffer(staging.buffer, m_meshlet_buffer.buffer, 1, &meshlet_copy);
            src_offset += meshlet_buffer_size;

            VkBufferCopy meshlet_vertices_copy{};
            meshlet_vertices_copy.dstOffset = 0;
            meshlet_vertices_copy.srcOffset = src_offset;
            meshlet_vertices_copy.size = meshlet_vertices_buffer_size;
            cmd.copy_buffer(staging.buffer, m_meshlet_vertices_buffer.buffer, 1, &meshlet_vertices_copy);
            src_offset += meshlet_vertices_buffer_size;

            VkBufferCopy meshlet_triangles_copy{};
            meshlet_triangles_copy.dstOffset = 0;
            meshlet_triangles_copy.srcOffset = src_offset;
            meshlet_triangles_copy.size = meshlet_triangles_buffer_size;
            cmd.copy_buffer(staging.buffer, m_meshlet_triangles_buffer.buffer, 1, &meshlet_triangles_copy);
        });

    device.destroy_buffer(staging);
}

Mesh::~Mesh()
{
    const Device& device = Engine::get().device();

    device.destroy_buffer(m_index_buffer);
    device.destroy_buffer(m_vertex_buffer);
    device.destroy_buffer(m_position_buffer);

    device.destroy_buffer(m_meshlet_buffer);
    device.destroy_buffer(m_meshlet_vertices_buffer);
    device.destroy_buffer(m_meshlet_triangles_buffer);
}

void Mesh::calculate_bounds(const std::span<glm::vec4>& positions)
{
    m_centroid = glm::vec3(0.f);
    for (const auto& v : positions) m_centroid += glm::vec3(v);
    m_centroid /= static_cast<float>(positions.size());

    m_radius = glm::distance2(glm::vec3(positions[0]), m_centroid);
    for (const auto& v : positions) m_radius = std::max(m_radius, glm::distance2(glm::vec3(v), m_centroid));
    m_radius = std::nextafter(sqrtf(m_radius), std::numeric_limits<float>::max());
}