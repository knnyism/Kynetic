//
// Created by kennypc on 11/13/25.
//

#include "mesh.hpp"

#include "core/device.hpp"
#include "core/engine.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

using namespace kynetic;

Mesh::Mesh(const std::filesystem::path& path,
           std::span<uint32_t> indices,
           std::span<Vertex> vertices,
           std::vector<Primitive> primitives)
    : Resource(Type::Mesh, path)
{
    Device& device = Engine::get().device();

    const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

    m_primitives = std::move(primitives);

    m_vertex_buffer = device.create_buffer(
        vertex_buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                  .buffer = m_vertex_buffer.buffer};
    m_vertex_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);
    m_index_buffer = device.create_buffer(index_buffer_size,
                                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          VMA_MEMORY_USAGE_GPU_ONLY);

    AllocatedBuffer staging = device.create_buffer(vertex_buffer_size + index_buffer_size,
                                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                   VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.allocation->GetMappedData();
    memcpy(data, vertices.data(), vertex_buffer_size);
    memcpy(static_cast<char*>(data) + vertex_buffer_size, indices.data(), index_buffer_size);

    device.immediate_submit(
        [&](VkCommandBuffer cmd)
        {
            VkBufferCopy vertex_copy{0};
            vertex_copy.dstOffset = 0;
            vertex_copy.srcOffset = 0;
            vertex_copy.size = vertex_buffer_size;

            vkCmdCopyBuffer(cmd, staging.buffer, m_vertex_buffer.buffer, 1, &vertex_copy);

            VkBufferCopy index_copy{0};
            index_copy.dstOffset = 0;
            index_copy.srcOffset = vertex_buffer_size;
            index_copy.size = index_buffer_size;

            vkCmdCopyBuffer(cmd, staging.buffer, m_index_buffer.buffer, 1, &index_copy);
        });

    device.destroy_buffer(staging);
}
Mesh::~Mesh()
{
    const Device& device = Engine::get().device();

    device.destroy_buffer(m_vertex_buffer);
    device.destroy_buffer(m_index_buffer);
}