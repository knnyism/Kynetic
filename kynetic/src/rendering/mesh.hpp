//
// Created by kennypc on 11/13/25.
//

#pragma once

#include "shader_types.hpp"

namespace kynetic
{
class Mesh : public Resource
{
    AllocatedBuffer m_index_buffer;
    AllocatedBuffer m_vertex_buffer;
    VkDeviceAddress m_vertex_buffer_address;

    VkIndexType m_index_type{VK_INDEX_TYPE_UINT32};

public:
    Mesh(const std::filesystem::path& path, std::span<uint32_t> indices, std::span<Vertex> vertices);
    ~Mesh() override;

    [[nodiscard]] const VkDeviceAddress& get_address() const { return m_vertex_buffer_address; }
    [[nodiscard]] const VkBuffer& get_indices() const { return m_index_buffer.buffer; }
    [[nodiscard]] VkIndexType get_index_type() const { return m_index_type; }
};
}  // namespace kynetic
