//
// Created by kenny on 11/16/25.
//

#include "descriptor_writer.hpp"

using namespace kynetic;

void DescriptorWriter::write_buffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& info =
        bufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = size});

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::write_image(uint32_t binding,
                                   VkImageView image,
                                   VkSampler sampler,
                                   VkImageLayout layout,
                                   VkDescriptorType type)
{
    VkDescriptorImageInfo& info =
        imageInfos.emplace_back(VkDescriptorImageInfo{.sampler = sampler, .imageView = image, .imageLayout = layout});

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void DescriptorWriter::clear()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : writes) write.dstSet = set;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
