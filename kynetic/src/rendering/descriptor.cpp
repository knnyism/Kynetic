//
// Created by kenny on 11/25/25.
//

#include "descriptor.hpp"

using namespace kynetic;

void DescriptorAllocator::init_pool(VkDevice device,
                                    const uint32_t max_sets,
                                    const std::span<PoolSizeRatio> pool_ratios,
                                    VkDescriptorPoolCreateFlags flags)
{
    m_device = device;

    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (const PoolSizeRatio ratio : pool_ratios)
    {
        pool_sizes.push_back(
            VkDescriptorPoolSize{.type = ratio.type,
                                 .descriptorCount = static_cast<uint32_t>(ratio.ratio * static_cast<float>(max_sets))});
    }

    VkDescriptorPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.maxSets = max_sets;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.flags = flags;

    vkCreateDescriptorPool(device, &pool_info, nullptr, &m_pool);
}

void DescriptorAllocator::clear_descriptors() const { vkResetDescriptorPool(m_device, m_pool, 0); }

void DescriptorAllocator::destroy_pool() const { vkDestroyDescriptorPool(m_device, m_pool, nullptr); }

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout, const void* pNext) const
{
    VkDescriptorSetAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = m_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout;
    alloc_info.pNext = pNext;

    VkDescriptorSet descriptor_set;
    VK_CHECK(vkAllocateDescriptorSets(m_device, &alloc_info, &descriptor_set));

    return descriptor_set;
}

void DescriptorAllocatorGrowable::init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> pool_ratios)
{
    m_device = device;

    m_ratios.clear();
    for (auto r : pool_ratios) m_ratios.push_back(r);

    VkDescriptorPool new_pool = create_pool(maxSets, pool_ratios);

    m_sets_per_pool = static_cast<uint32_t>(static_cast<float>(maxSets) * 1.5f);  // grow it next allocation

    m_ready_pools.push_back(new_pool);
}

void DescriptorAllocatorGrowable::clear_descriptors()
{
    for (auto* pool : m_ready_pools)
    {
        vkResetDescriptorPool(m_device, pool, 0);
    }
    for (auto* pool : m_full_pools)
    {
        vkResetDescriptorPool(m_device, pool, 0);
        m_ready_pools.push_back(pool);
    }
    m_full_pools.clear();
}

void DescriptorAllocatorGrowable::destroy_pool()
{
    for (auto* pool : m_ready_pools) vkDestroyDescriptorPool(m_device, pool, nullptr);
    m_ready_pools.clear();

    for (auto* pool : m_full_pools) vkDestroyDescriptorPool(m_device, pool, nullptr);
    m_full_pools.clear();
}

VkDescriptorPool DescriptorAllocatorGrowable::get_pool()
{
    VkDescriptorPool new_pool;
    if (!m_ready_pools.empty())
    {
        new_pool = m_ready_pools.back();
        m_ready_pools.pop_back();
    }
    else
    {
        new_pool = create_pool(m_sets_per_pool, m_ratios);

        m_sets_per_pool = static_cast<uint32_t>(static_cast<float>(m_sets_per_pool) * 1.5f);
        if (m_sets_per_pool > 4092) m_sets_per_pool = 4092;
    }

    return new_pool;
}

VkDescriptorPool DescriptorAllocatorGrowable::create_pool(uint32_t set_count, std::span<PoolSizeRatio> pool_ratios) const
{
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (PoolSizeRatio ratio : pool_ratios)
    {
        pool_sizes.push_back(
            VkDescriptorPoolSize{.type = ratio.type,
                                 .descriptorCount = static_cast<uint32_t>(ratio.ratio * static_cast<float>(set_count))});
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = set_count;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(m_device, &pool_info, nullptr, &newPool);
    return newPool;
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDescriptorSetLayout layout, const void* pNext)
{
    VkDescriptorPool pool_to_use = get_pool();

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.pNext = pNext;
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool_to_use;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout;

    VkDescriptorSet descriptor_set;
    VkResult result = vkAllocateDescriptorSets(m_device, &alloc_info, &descriptor_set);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        m_full_pools.push_back(pool_to_use);

        pool_to_use = get_pool();
        alloc_info.descriptorPool = pool_to_use;

        VK_CHECK(vkAllocateDescriptorSets(m_device, &alloc_info, &descriptor_set));
    }

    m_ready_pools.push_back(pool_to_use);
    return descriptor_set;
}

void DescriptorWriter::write_buffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& info =
        m_buffer_infos.emplace_back(VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = size});

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    m_writes.push_back(write);
}

void DescriptorWriter::write_image(uint32_t binding,
                                   VkImageView image,
                                   VkSampler sampler,
                                   VkImageLayout layout,
                                   VkDescriptorType type,
                                   uint32_t array_element)
{
    VkDescriptorImageInfo& info =
        m_image_infos.emplace_back(VkDescriptorImageInfo{.sampler = sampler, .imageView = image, .imageLayout = layout});

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.descriptorCount = 1;
    write.dstArrayElement = array_element;
    write.descriptorType = type;
    write.dstSet = VK_NULL_HANDLE;
    write.dstBinding = binding;
    write.pImageInfo = &info;

    m_writes.push_back(write);
}

void DescriptorWriter::clear()
{
    m_image_infos.clear();
    m_writes.clear();
    m_buffer_infos.clear();
}

void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : m_writes) write.dstSet = set;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);
}

void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type, uint32_t descriptor_count)
{
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding = binding;
    newbind.descriptorCount = descriptor_count;
    newbind.descriptorType = type;

    m_bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear() { m_bindings.clear(); }

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device,
                                                     VkShaderStageFlags shader_stages,
                                                     void* pNext,
                                                     VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : m_bindings) b.stageFlags |= shader_stages;

    VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = pNext;

    info.pBindings = m_bindings.data();
    info.bindingCount = static_cast<uint32_t>(m_bindings.size());
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}