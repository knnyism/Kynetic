//
// Created by kenny on 11/6/25.
//

#include "device.hpp"
#include "engine.hpp"

#include "rendering/texture.hpp"
#include "rendering/mesh.hpp"
#include "rendering/material.hpp"

#include "resource_manager.hpp"

using namespace kynetic;
ResourceManager::ResourceManager()
{
    VkSamplerCreateInfo nearest_sampler{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
    };

    uint32_t white = glm::packUnorm4x8(glm::vec4(1.f, 1.f, 1.f, 1.f));
    m_default_textures.push_back(load<Texture>("dev/white",
                                               &white,
                                               VkExtent3D{1, 1, 1},
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                                               nearest_sampler));

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.5f, 0.5f, 1.f, 1.f));
    m_default_textures.push_back(load<Texture>("dev/normal",
                                               &grey,
                                               VkExtent3D{1, 1, 1},
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                                               nearest_sampler));
    uint32_t black = glm::packUnorm4x8(glm::vec4(0.f, 0.f, 0.f, 0.f));
    m_default_textures.push_back(load<Texture>("dev/black",
                                               &black,
                                               VkExtent3D{1, 1, 1},
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                                               nearest_sampler));

    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1.f, 0.f, 1.f, 1.f));

    std::array<uint32_t, 16 * 16> missing;
    for (uint32_t x = 0; x < 16; x++)
        for (uint32_t y = 0; y < 16; y++) missing[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;

    m_default_textures.push_back(load<Texture>("dev/missing",
                                               &missing,
                                               VkExtent3D{16, 16, 1},
                                               VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_USAGE_SAMPLED_BIT,
                                               nearest_sampler));
}
ResourceManager::~ResourceManager()
{
    Device& device = Engine::get().device();

    if (m_merged_vertex_buffer.buffer != VK_NULL_HANDLE) device.destroy_buffer(m_merged_vertex_buffer);
    if (m_merged_index_buffer.buffer != VK_NULL_HANDLE) device.destroy_buffer(m_merged_index_buffer);
    if (m_material_buffer.buffer != VK_NULL_HANDLE) device.destroy_buffer(m_material_buffer);
}

void ResourceManager::refresh_mesh_buffers()
{
    Device& device = Engine::get().device();

    if (m_merged_vertex_buffer.buffer != VK_NULL_HANDLE) device.destroy_buffer(m_merged_vertex_buffer);
    if (m_merged_index_buffer.buffer != VK_NULL_HANDLE) device.destroy_buffer(m_merged_index_buffer);

    size_t total_vertices = 0;
    size_t total_indices = 0;

    for_each<Mesh>(
        [&](const std::shared_ptr<Mesh>& mesh)
        {
            mesh->m_first_index = static_cast<uint32_t>(total_indices);
            mesh->m_first_vertex = static_cast<uint32_t>(total_vertices);

            total_vertices += mesh->m_vertex_count;
            total_indices += mesh->m_index_count;
        });

    m_merged_index_buffer = device.create_buffer(
        total_indices * sizeof(uint32_t),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    m_merged_vertex_buffer = device.create_buffer(
        total_vertices * sizeof(Vertex),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                  .buffer = m_merged_vertex_buffer.buffer};
    m_merged_vertex_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);

    device.immediate_submit(
        [&](const CommandBuffer& cmd)
        {
            for_each<Mesh>(
                [&](const std::shared_ptr<Mesh>& mesh)
                {
                    VkBufferCopy vertexCopy;
                    vertexCopy.dstOffset = mesh->m_first_vertex * sizeof(Vertex);
                    vertexCopy.size = mesh->m_vertex_count * sizeof(Vertex);
                    vertexCopy.srcOffset = 0;

                    cmd.copy_buffer(mesh->m_vertex_buffer.buffer, m_merged_vertex_buffer.buffer, 1, &vertexCopy);

                    VkBufferCopy indexCopy;
                    indexCopy.dstOffset = mesh->m_first_index * sizeof(uint32_t);
                    indexCopy.size = mesh->m_index_count * sizeof(uint32_t);
                    indexCopy.srcOffset = 0;

                    cmd.copy_buffer(mesh->m_index_buffer.buffer, m_merged_index_buffer.buffer, 1, &indexCopy);
                });
        });
}

void ResourceManager::refresh_material_buffer()
{
    Device& device = Engine::get().device();

    if (m_material_buffer.buffer != VK_NULL_HANDLE) device.destroy_buffer(m_material_buffer);

    uint32_t material_count = 0;

    std::vector<MaterialData> material_datas;

    for_each<Material>(
        [&](const std::shared_ptr<Material>& material)
        {
            material->m_handle = material_count++;

            MaterialData& material_data = material_datas.emplace_back();
            material_data.albedo = material->m_albedo->m_handle;
            material_data.normal = material->m_normal->m_handle;
            material_data.metal_rough = material->m_metal_roughness->m_handle;
            material_data.emissive = material->m_emissive->m_handle;
        });

    m_material_buffer = device.create_buffer(
        material_count * sizeof(MaterialData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    VkBufferDeviceAddressInfo device_address_info{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                  .buffer = m_material_buffer.buffer};
    m_material_buffer_address = vkGetBufferDeviceAddress(device.get(), &device_address_info);

    size_t material_buffer_size = material_datas.size() * sizeof(MaterialData);

    AllocatedBuffer staging =
        device.create_buffer(material_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(device.get_allocator(), staging.allocation, &data);
    memcpy(data, material_datas.data(), material_buffer_size);
    vmaUnmapMemory(device.get_allocator(), staging.allocation);

    device.immediate_submit(
        [&](const CommandBuffer& cmd)
        {
            VkBufferCopy copy;
            copy.dstOffset = 0;
            copy.size = material_buffer_size;
            copy.srcOffset = 0;

            cmd.copy_buffer(staging.buffer, m_material_buffer.buffer, 1, &copy);
        });

    device.destroy_buffer(staging);
}

void ResourceManager::refresh_bindless_textures()
{
    Device& device = Engine::get().device();

    uint32_t texture_index = 0;

    DescriptorWriter writer;
    for_each<Texture>(
        [&](const std::shared_ptr<Texture>& texture)
        {
            texture->m_handle = texture_index++;

            writer.write_image(0,
                               texture->m_image.view,
                               texture->m_sampler,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               texture->m_handle);
        });

    writer.update_set(device.get(), device.get_bindless_set());
}