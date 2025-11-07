//
// Created by kennypc on 11/7/25.
//

#include "core/device.hpp"
#include "core/engine.hpp"
#include "pipeline.hpp"
#include "shader.hpp"

using namespace kynetic;

Shader::Shader(VkDevice device, std::shared_ptr<ShaderResource> shader_resource)
    : m_resource(std::move(shader_resource)), m_device(device)
{
    m_pipeline = ComputePipelineBuilder().set_shader(m_resource).build(m_device);

    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4},
    };
    m_descriptor_allocator.init_pool(m_device, 1000, sizes);

    for (const auto& binding_map = m_resource->get_binding_map(); const auto& [name, info] : binding_map)
    {
        ResourceBinding binding;
        binding.set = info.set;
        binding.binding = info.binding;
        binding.type = info.type;
        binding.is_bound = false;

        m_bindings[name] = binding;
    }

    uint32_t max_set = 0;
    for (const auto& binding : m_bindings | std::views::values) max_set = std::max(max_set, binding.set);

    m_descriptor_sets.resize(max_set + 1, VK_NULL_HANDLE);
    for (uint32_t i = 0; i <= max_set; ++i)
    {
        if (VkDescriptorSetLayout layout = m_resource->get_layout_at(i); layout != VK_NULL_HANDLE)
        {
            m_descriptor_sets[i] = m_descriptor_allocator.allocate(m_device, layout);
        }
    }
}

Shader::~Shader()
{
    if (m_pipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_device, m_pipeline, nullptr);
    m_descriptor_allocator.destroy_pool(m_device);
}

Shader::Shader(Shader&& other) noexcept
    : m_resource(std::move(other.m_resource)),
      m_pipeline(other.m_pipeline),
      m_device(other.m_device),
      m_descriptor_allocator(std::move(other.m_descriptor_allocator)),
      m_descriptor_sets(std::move(other.m_descriptor_sets)),
      m_bindings(std::move(other.m_bindings)),
      m_pending_writes(std::move(other.m_pending_writes))
{
    other.m_pipeline = VK_NULL_HANDLE;
    other.m_device = VK_NULL_HANDLE;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        if (m_pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
        }
        m_descriptor_allocator.destroy_pool(m_device);

        m_resource = std::move(other.m_resource);
        m_pipeline = other.m_pipeline;
        m_device = other.m_device;
        m_descriptor_allocator = std::move(other.m_descriptor_allocator);
        m_descriptor_sets = std::move(other.m_descriptor_sets);
        m_bindings = std::move(other.m_bindings);
        m_pending_writes = std::move(other.m_pending_writes);

        other.m_pipeline = VK_NULL_HANDLE;
        other.m_device = VK_NULL_HANDLE;
    }
    return *this;
}

void Shader::update_descriptors()
{
    for (auto& writes : m_pending_writes | std::views::values)
    {
        if (writes.empty()) continue;

        std::vector<VkWriteDescriptorSet> vk_writes;
        vk_writes.reserve(writes.size());
        for (auto& pending : writes) vk_writes.push_back(pending.write);

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(vk_writes.size()), vk_writes.data(), 0, nullptr);
    }

    m_pending_writes.clear();
}

void Shader::set_image(const char* name, VkImageView view, const VkImageLayout layout)
{
    const auto it = m_bindings.find(name);
    if (it == m_bindings.end())
    {
        fmt::println(stderr, "Binding '{}' not found in shader", name);
        return;
    }

    auto& [set, binding, type, is_bound] = it->second;

    PendingWrite& pending = m_pending_writes[set].emplace_back();
    pending.image_info.imageView = view;
    pending.image_info.imageLayout = layout;
    pending.image_info.sampler = VK_NULL_HANDLE;

    pending.write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    pending.write.dstSet = m_descriptor_sets[set];
    pending.write.dstBinding = binding;
    pending.write.descriptorCount = 1;
    pending.write.descriptorType = type;
    pending.write.pImageInfo = &pending.image_info;

    is_bound = true;
}

// void Shader::set_buffer(const char* name, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {}

void Shader::set_push_constants(VkCommandBuffer command_buffer,
                                const uint32_t size,
                                const void* data,
                                const uint32_t offset) const
{
    vkCmdPushConstants(command_buffer, m_resource->get_layout(), VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
}

void Shader::dispatch(VkCommandBuffer command_buffer, uint32_t x, uint32_t y, uint32_t z)
{
    update_descriptors();

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    if (!m_descriptor_sets.empty())
        vkCmdBindDescriptorSets(command_buffer,
                                VK_PIPELINE_BIND_POINT_COMPUTE,
                                m_resource->get_layout(),
                                0,
                                static_cast<uint32_t>(m_descriptor_sets.size()),
                                m_descriptor_sets.data(),
                                0,
                                nullptr);

    vkCmdDispatch(command_buffer, x, y, z);
}