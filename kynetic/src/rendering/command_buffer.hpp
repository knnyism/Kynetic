//
// Created by kennypc on 11/7/25.
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

    void begin_rendering(const VkRenderingInfo& rendering_info) const;
    void end_rendering() const;

    void set_viewport(float width, float height) const;
    void set_scissor(uint32_t width, uint32_t height) const;

    void bind_pipeline(Pipeline* pipeline);
    void bind_descriptors(VkDescriptorSet descriptor_set) const;
    void set_push_constants(VkShaderStageFlagBits stage_flags, uint32_t size, const void* data, uint32_t offset = 0) const;

    void dispatch(uint32_t x, uint32_t y, uint32_t z) const;

    void draw_auto(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) const;
};

}  // namespace kynetic