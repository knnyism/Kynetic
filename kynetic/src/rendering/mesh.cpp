//
// Created by kenny on 11/13/25.
//

#include "mesh.hpp"

#include <utility>

#include "core/device.hpp"
#include "core/engine.hpp"

#include "vma_usage.hpp"
#include "glm/gtx/norm.hpp"

KX_DISABLE_WARNING_PUSH
KX_DISABLE_WARNING_SIGNED_UNSIGNED_ASSIGNMENT_MISMATCH
#include "meshoptimizer.h"

#define CLUSTERLOD_IMPLEMENTATION
#include "clusterlod.h"

KX_DISABLE_WARNING_POP

using namespace kynetic;

Mesh::Mesh(const std::filesystem::path& path,
           uint32_t mesh_index,
           std::span<uint32_t> unindexed_indices,
           std::span<glm::vec4> unindexed_positions,
           std::span<Vertex> unindexed_vertices,
           std::shared_ptr<Material> material)
    : Resource(Type::Mesh, path.string()),
      m_mesh_index(mesh_index),
      m_index_count(static_cast<uint32_t>(unindexed_indices.size())),
      m_vertex_count(static_cast<uint32_t>(unindexed_vertices.size())),
      m_material(std::move(material))
{
    std::span<glm::vec4>& positions = unindexed_positions;
    std::span<uint32_t>& indices = unindexed_indices;

    calculate_bounds(unindexed_positions);

    clodConfig config = clodDefaultConfig(126);
    config.max_vertices = 64;

    clodMesh mesh{};
    mesh.indices = indices.data();
    mesh.index_count = indices.size();
    mesh.vertex_count = positions.size();
    mesh.vertex_positions = reinterpret_cast<const float*>(positions.data());
    mesh.vertex_positions_stride = sizeof(glm::vec4);

    mesh.vertex_attributes = reinterpret_cast<const float*>(unindexed_vertices.data());
    mesh.vertex_attributes_stride = sizeof(Vertex);
    mesh.attribute_weights = VERTEX_ATTRIBUTE_WEIGHTS;
    mesh.attribute_count = VERTEX_ATTRIBUTE_COUNT;
    mesh.attribute_protect_mask = 1u << 3 | 1u << 12;
    mesh.vertex_lock = nullptr;

    std::vector<MeshletData> meshlets;
    std::vector<LODGroupData> lod_groups;
    std::vector<uint32_t> meshlet_vertices;
    std::vector<uint8_t> meshlet_triangles;

    std::vector<std::vector<int>> child_groups;

    uint32_t current_vertex_offset{0};
    uint32_t current_triangle_offset{0};
    uint32_t max_depth{0};

    clodBuild(config,
              mesh,
              [&](clodGroup group, const clodCluster* clusters, size_t cluster_count) -> int
              {
                  int group_id = static_cast<int>(lod_groups.size());

                  LODGroupData lod_group{};
                  lod_group.center =
                      glm::vec3(group.simplified.center[0], group.simplified.center[1], group.simplified.center[2]);
                  lod_group.radius = group.simplified.radius;
                  lod_group.error = group.simplified.error;
                  lod_group.depth = group.depth;
                  lod_group.cluster_start = static_cast<uint32_t>(meshlets.size());
                  lod_group.cluster_count = static_cast<uint32_t>(cluster_count);

                  lod_groups.push_back(lod_group);

                  for (size_t i = 0; i < cluster_count; ++i)
                  {
                      const clodCluster& cluster = clusters[i];

                      MeshletData meshlet{};

                      meshlet.center = glm::vec3(cluster.bounds.center[0], cluster.bounds.center[1], cluster.bounds.center[2]);
                      meshlet.radius = cluster.bounds.radius;

                      meshlet.group_id = group_id;
                      meshlet.parent_group_id = cluster.refined;
                      meshlet.lod_level = static_cast<uint8_t>(group.depth);
                      meshlet.lod_error = cluster.bounds.error;
                      meshlet.parent_error = group.simplified.error;

                      size_t triangle_count = cluster.index_count / 3;
                      std::vector<unsigned int> local_vertices(cluster.vertex_count);
                      std::vector<unsigned char> local_triangles(cluster.index_count);

                      size_t unique_verts =
                          clodLocalIndices(local_vertices.data(), local_triangles.data(), cluster.indices, cluster.index_count);

                      meshlet.vertex_offset = current_vertex_offset;
                      meshlet.triangle_offset = current_triangle_offset;
                      meshlet.vertex_count = static_cast<uint8_t>(std::min(unique_verts, static_cast<size_t>(255)));
                      meshlet.triangle_count = static_cast<uint8_t>(std::min(triangle_count, static_cast<size_t>(255)));

                      for (size_t v = 0; v < unique_verts; ++v) meshlet_vertices.push_back(local_vertices[v]);
                      current_vertex_offset += static_cast<uint32_t>(unique_verts);

                      for (size_t t = 0; t < cluster.index_count; ++t) meshlet_triangles.push_back(local_triangles[t]);
                      current_triangle_offset += static_cast<uint32_t>(cluster.index_count);

                      meshlets.push_back(meshlet);
                  }

                  return group_id;
              });

    m_meshlet_count = meshlets.size();
    m_lod_group_count = lod_groups.size();
    m_max_lod_level = max_depth;

    fmt::print("Mesh '{}': {} clusters, {} LOD groups, {} levels\n",
               path.string(),
               m_meshlet_count,
               m_lod_group_count,
               m_max_lod_level + 1);

    for (uint32_t level = 0; level <= m_max_lod_level; ++level)
    {
        float min_error = FLT_MAX, max_error = 0.0f;
        float min_parent = FLT_MAX, max_parent = 0.0f;
        uint32_t count = 0;
        for (const auto& m : meshlets)
        {
            if (m.lod_level == level)
            {
                min_error = std::min(min_error, m.lod_error);
                max_error = std::max(max_error, m.lod_error);
                min_parent = std::min(min_parent, m.parent_error);
                max_parent = std::max(max_parent, m.parent_error);
                count++;
            }
        }
        fmt::print("  LOD {}: {} clusters, error=[{:.4f}, {:.4f}], parent_error=[{:.4f}, {:.4f}]\n",
                   level,
                   count,
                   min_error,
                   max_error,
                   min_parent,
                   max_parent);
    }

    for (auto& meshlet : meshlets)
    {
        if (meshlet.triangle_count == 0) continue;

        std::vector<unsigned int> meshlet_indices;
        meshlet_indices.reserve(meshlet.triangle_count * 3);

        for (uint32_t t = 0; t < meshlet.triangle_count * 3; ++t)
        {
            uint8_t local_idx = meshlet_triangles[meshlet.triangle_offset + t];
            uint32_t global_idx = meshlet_vertices[meshlet.vertex_offset + local_idx];
            meshlet_indices.push_back(global_idx);
        }

        meshopt_Bounds bounds = meshopt_computeClusterBounds(meshlet_indices.data(),
                                                             meshlet_indices.size(),
                                                             reinterpret_cast<const float*>(positions.data()),
                                                             positions.size(),
                                                             sizeof(glm::vec4));

        meshlet.cone_axis[0] = bounds.cone_axis_s8[0];
        meshlet.cone_axis[1] = bounds.cone_axis_s8[1];
        meshlet.cone_axis[2] = bounds.cone_axis_s8[2];
        meshlet.cone_cutoff = bounds.cone_cutoff_s8;
    }

    Device& device = Engine::get().device();

    const size_t index_buffer_size = indices.size() * sizeof(uint32_t);
    const size_t position_buffer_size = positions.size() * sizeof(glm::vec4);
    const size_t vertex_buffer_size = unindexed_vertices.size() * sizeof(Vertex);

    const size_t meshlet_buffer_size = meshlets.size() * sizeof(MeshletData);
    const size_t meshlet_vertices_buffer_size = meshlet_vertices.size() * sizeof(uint32_t);
    const size_t meshlet_triangles_buffer_size = meshlet_triangles.size() * sizeof(uint8_t);
    const size_t lod_groups_buffer_size = lod_groups.size() * sizeof(LODGroupData);

    m_index_buffer = device.create_buffer(index_buffer_size,
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

    m_lod_groups_buffer = device.create_buffer(lod_groups_buffer_size,
                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                               VMA_MEMORY_USAGE_GPU_ONLY);
    {
        VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                      .buffer = m_lod_groups_buffer.buffer};
        m_lod_groups_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    }

    const size_t total_staging_size = index_buffer_size + position_buffer_size + vertex_buffer_size + meshlet_buffer_size +
                                      meshlet_vertices_buffer_size + meshlet_triangles_buffer_size + lod_groups_buffer_size;

    AllocatedBuffer staging =
        device.create_buffer(total_staging_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);

    size_t offset = 0;
    memcpy(static_cast<char*>(data) + offset, indices.data(), index_buffer_size);
    offset += index_buffer_size;

    memcpy(static_cast<char*>(data) + offset, positions.data(), position_buffer_size);
    offset += position_buffer_size;

    memcpy(static_cast<char*>(data) + offset, unindexed_vertices.data(), vertex_buffer_size);
    offset += vertex_buffer_size;

    memcpy(static_cast<char*>(data) + offset, meshlets.data(), meshlet_buffer_size);
    offset += meshlet_buffer_size;

    memcpy(static_cast<char*>(data) + offset, meshlet_vertices.data(), meshlet_vertices_buffer_size);
    offset += meshlet_vertices_buffer_size;

    memcpy(static_cast<char*>(data) + offset, meshlet_triangles.data(), meshlet_triangles_buffer_size);
    offset += meshlet_triangles_buffer_size;

    memcpy(static_cast<char*>(data) + offset, lod_groups.data(), lod_groups_buffer_size);

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
            src_offset += meshlet_triangles_buffer_size;

            VkBufferCopy lod_groups_copy{};
            lod_groups_copy.dstOffset = 0;
            lod_groups_copy.srcOffset = src_offset;
            lod_groups_copy.size = lod_groups_buffer_size;
            cmd.copy_buffer(staging.buffer, m_lod_groups_buffer.buffer, 1, &lod_groups_copy);
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
    device.destroy_buffer(m_lod_groups_buffer);
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