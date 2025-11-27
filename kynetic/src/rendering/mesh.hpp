//
// Created by kenny on 11/13/25.
//

#pragma once

namespace kynetic
{

class Mesh : public Resource
{
    friend class ResourceManager;

    AllocatedBuffer m_index_buffer;
    AllocatedBuffer m_vertex_buffer;

    uint32_t m_first_index{0};
    uint32_t m_index_count{0};

    uint32_t m_first_vertex{0};
    uint32_t m_vertex_count{0};

    VkIndexType m_index_type{VK_INDEX_TYPE_UINT32};
    VkDeviceAddress m_vertex_buffer_address;

    std::shared_ptr<class Material> m_material;

    glm::vec3 m_centroid{0.f};
    float m_radius{0.0f};
    void calculate_radius(const std::span<Vertex>& vertices);

public:
    Mesh(const std::filesystem::path& path,
         std::span<uint32_t> indices,
         std::span<Vertex> vertices,
         std::shared_ptr<Material> material);
    ~Mesh() override;

    [[nodiscard]] const VkBuffer& get_indices() const { return m_index_buffer.buffer; }
    [[nodiscard]] const VkBuffer& get_vertices() const { return m_vertex_buffer.buffer; }
    [[nodiscard]] VkIndexType get_index_type() const { return m_index_type; }

    [[nodiscard]] uint32_t get_index_offset() const { return m_first_index; }
    [[nodiscard]] uint32_t get_index_count() const { return m_index_count; }

    [[nodiscard]] uint32_t get_vertex_offset() const { return m_first_vertex; }
    [[nodiscard]] uint32_t get_vertex_count() const { return m_vertex_count; }

    [[nodiscard]] VkDeviceAddress get_vertex_buffer_address() const { return m_vertex_buffer_address; }

    [[nodiscard]] const std::shared_ptr<Material>& get_material() const { return m_material; }

    [[nodiscard]] glm::vec3 get_centroid() const { return m_centroid; }
    [[nodiscard]] float get_radius() const { return m_radius; }
};

}  // namespace kynetic
