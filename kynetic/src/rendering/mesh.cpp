//
// Created by kenny on 11/13/25.
//

#include "mesh.hpp"

#include <utility>

#include "core/device.hpp"
#include "core/engine.hpp"

#include "vma_usage.hpp"
#include <glm/gtx/norm.hpp>

using namespace kynetic;

void Mesh::calculate_radius(const std::span<Vertex>& vertices)
{
    m_centroid = glm::vec3(0.f);
    for (const auto& v : vertices) m_centroid += v.position;
    m_centroid /= static_cast<float>(vertices.size());

    m_radius = glm::distance2(vertices[0].position, m_centroid);
    for (const auto& v : vertices) m_radius = std::max(m_radius, glm::distance2(v.position, m_centroid));
    m_radius = std::nextafter(sqrtf(m_radius), std::numeric_limits<float>::max());
}

Mesh::Mesh(const std::filesystem::path& path,
           std::span<uint32_t> indices,
           std::span<Vertex> vertices,
           std::shared_ptr<Material> material)
    : Resource(Type::Mesh, path.string()),
      m_index_count(static_cast<uint32_t>(indices.size())),
      m_vertex_count(static_cast<uint32_t>(vertices.size())),
      m_material(std::move(material))
{
    calculate_radius(vertices);

    Device& device = Engine::get().device();

    const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

    m_index_buffer = device.create_buffer(
        index_buffer_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    m_vertex_buffer = device.create_buffer(vertex_buffer_size,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                  .buffer = m_vertex_buffer.buffer};
    m_vertex_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);

    AllocatedBuffer staging = device.create_buffer(vertex_buffer_size + index_buffer_size,
                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                   VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);
    memcpy(data, vertices.data(), vertex_buffer_size);
    memcpy(static_cast<char*>(data) + vertex_buffer_size, indices.data(), index_buffer_size);
    vmaUnmapMemory(device.get_allocator(), staging.allocation);

    device.immediate_submit(
        [&](const CommandBuffer& cmd)
        {
            VkBufferCopy vertex_copy{};
            vertex_copy.dstOffset = 0;
            vertex_copy.srcOffset = 0;
            vertex_copy.size = vertex_buffer_size;

            cmd.copy_buffer(staging.buffer, m_vertex_buffer.buffer, 1, &vertex_copy);

            VkBufferCopy index_copy{};
            index_copy.dstOffset = 0;
            index_copy.srcOffset = vertex_buffer_size;
            index_copy.size = index_buffer_size;

            cmd.copy_buffer(staging.buffer, m_index_buffer.buffer, 1, &index_copy);
        });

    device.destroy_buffer(staging);
}

Mesh::~Mesh()
{
    const Device& device = Engine::get().device();

    device.destroy_buffer(m_vertex_buffer);
    device.destroy_buffer(m_index_buffer);
}