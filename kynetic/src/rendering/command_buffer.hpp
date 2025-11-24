//
// Created by kenny on 11/7/25.
//

#pragma once

namespace kynetic
{

class CommandBuffer
{
    friend class Device;

    VkDevice m_device{VK_NULL_HANDLE};
    VkCommandPool m_command_pool{VK_NULL_HANDLE};
    VkCommandBuffer m_command_buffer{VK_NULL_HANDLE};

    class Pipeline* m_current_pipeline{nullptr};

public:
    void init(VkDevice device, uint32_t queue_family_index);
    void shutdown() const;

    void reset();
    void transition_image(VkImage image, VkImageLayout current_layout, VkImageLayout new_layout) const;
    void copy_image_to_image(VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) const;
    void copy_buffer_to_image(VkBuffer srcBuffer,
                              VkImage dstImage,
                              VkImageLayout dstImageLayout,
                              uint32_t regionCount,
                              const VkBufferImageCopy* pRegions) const;
    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const;

    void begin_rendering(const VkRenderingInfo& rendering_info) const;
    void end_rendering() const;

    void set_viewport(float width, float height) const;
    void set_scissor(uint32_t width, uint32_t height) const;

    void bind_pipeline(Pipeline* pipeline);
    void bind_descriptors(VkDescriptorSet descriptor_set) const;
    void bind_vertex_buffer(uint32_t first_binding,
                            uint32_t binding_count,
                            VkBuffer vertex_buffer,
                            VkDeviceSize offset = 0) const;
    void bind_index_buffer(VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset = 0) const;
    void set_push_constants(VkShaderStageFlags stage_flage, uint32_t size, const void* data, uint32_t offset = 0) const;

    void dispatch(uint32_t x, uint32_t y, uint32_t z) const;

    void draw_auto(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const;
    void draw(uint32_t index_count,
              uint32_t instance_count,
              uint32_t first_index,
              int32_t vertex_offset,
              uint32_t first_instance) const;
    void multi_draw_indirect(VkBuffer buffer, uint32_t draw_count, uint32_t stride, VkDeviceSize offset = 0) const;
};

}  // namespace kynetic