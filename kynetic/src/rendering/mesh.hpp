//
// Created by kennypc on 11/13/25.
//

#pragma once

namespace kynetic
{

struct Primitive
{
    uint32_t first;
    uint32_t count;
};

class Mesh : public Resource
{
    AllocatedBuffer m_index_buffer;
    AllocatedBuffer m_vertex_buffer;
    VkDeviceAddress m_vertex_buffer_address;

    VkIndexType m_index_type{VK_INDEX_TYPE_UINT32};

    std::vector<Primitive> m_primitives;

public:
    Mesh(const std::filesystem::path& path,
         std::span<uint32_t> indices,
         std::span<Vertex> vertices,
         std::vector<Primitive> primitives);
    ~Mesh() override;

    [[nodiscard]] const VkDeviceAddress& get_address() const { return m_vertex_buffer_address; }
    [[nodiscard]] const VkBuffer& get_indices() const { return m_index_buffer.buffer; }
    [[nodiscard]] VkIndexType get_index_type() const { return m_index_type; }

    [[nodiscard]] const std::vector<Primitive>& get_primitives() { return m_primitives; }
};

}  // namespace kynetic
