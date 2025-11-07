//
// Created by kennypc on 11/7/25.
//

#pragma once

#include "resource/shader.hpp"

namespace kynetic
{
class Shader
{
    struct ResourceBinding
    {
        uint32_t set;
        uint32_t binding;
        VkDescriptorType type;
        bool is_bound = false;
    };

    struct PendingWrite
    {
        VkWriteDescriptorSet write;
        VkDescriptorImageInfo image_info;
        VkDescriptorBufferInfo buffer_info;
    };

    std::shared_ptr<ShaderResource> m_resource;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    DescriptorAllocator m_descriptor_allocator;
    std::vector<VkDescriptorSet> m_descriptor_sets;

    std::unordered_map<std::string, ResourceBinding> m_bindings;
    std::map<uint32_t, std::vector<PendingWrite>> m_pending_writes;

    void update_descriptors();

public:
    Shader(VkDevice device, std::shared_ptr<ShaderResource> shader_resource);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void set_image(const char* name, VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL);
    void set_buffer(const char* name, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

    void set_push_constants(VkCommandBuffer command_buffer, uint32_t size, const void* data, uint32_t offset = 0) const;

    void dispatch(VkCommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t z);
};
}  // namespace kynetic