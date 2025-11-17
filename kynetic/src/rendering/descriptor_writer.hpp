//
// Created by kenny on 11/16/25.
//

#pragma once

namespace kynetic
{

class DescriptorWriter
{
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

public:
    void write_image(uint32_t binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void write_buffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void clear();

    void update_set(VkDevice device, VkDescriptorSet set);
};

}  // namespace kynetic
