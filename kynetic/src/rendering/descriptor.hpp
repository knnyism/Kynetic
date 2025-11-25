//
// Created by kenny on 11/25/25.
//

#pragma once

namespace kynetic
{

struct PoolSizeRatio
{
    VkDescriptorType type;
    float ratio;
};

class DescriptorAllocator
{
protected:
    VkDevice m_device{VK_NULL_HANDLE};
    VkDescriptorPool m_pool{VK_NULL_HANDLE};

public:
    void init_pool(VkDevice device,
                   uint32_t max_sets,
                   std::span<PoolSizeRatio> pool_ratios,
                   VkDescriptorPoolCreateFlags flags = 0);
    void clear_descriptors() const;
    void destroy_pool() const;

    VkDescriptorSet allocate(VkDescriptorSetLayout layout, const void* pNext = nullptr) const;
};

class DescriptorAllocatorGrowable : public DescriptorAllocator
{
    VkDescriptorPool get_pool();
    VkDescriptorPool create_pool(uint32_t set_count, std::span<PoolSizeRatio> pool_ratios) const;

    std::vector<PoolSizeRatio> m_ratios;
    std::vector<VkDescriptorPool> m_full_pools;
    std::vector<VkDescriptorPool> m_ready_pools;
    uint32_t m_sets_per_pool{0};

public:
    void init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    void clear_descriptors();
    void destroy_pool();

    VkDescriptorSet allocate(VkDescriptorSetLayout layout, const void* pNext = nullptr);
};

class DescriptorWriter
{
    std::deque<VkDescriptorImageInfo> m_image_infos;
    std::deque<VkDescriptorBufferInfo> m_buffer_infos;
    std::vector<VkWriteDescriptorSet> m_writes;

public:
    void write_image(uint32_t binding,
                     VkImageView image,
                     VkSampler sampler,
                     VkImageLayout layout,
                     VkDescriptorType type,
                     uint32_t array_element = 0);
    void write_buffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void clear();

    void update_set(VkDevice device, VkDescriptorSet set);
};

class DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> m_bindings;

public:
    void add_binding(uint32_t binding, VkDescriptorType type, uint32_t descriptor_count = 1);
    void clear();

    VkDescriptorSetLayout build(VkDevice device,
                                VkShaderStageFlags stage_flags,
                                void* pNext = nullptr,
                                VkDescriptorSetLayoutCreateFlags flags = 0);
};

}  // namespace kynetic
