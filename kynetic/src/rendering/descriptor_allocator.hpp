//
// Created by kenny on 11/15/25.
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
    void init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    void clear_descriptors() const;
    void destroy_pool() const;

    VkDescriptorSet allocate(VkDescriptorSetLayout layout) const;
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

}  // namespace kynetic