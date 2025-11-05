//
// Created by kennypc on 11/4/25.
//

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <thread>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define VK_CHECK(x)                                                        \
    do                                                                     \
    {                                                                      \
        VkResult err = x;                                                  \
        if (err)                                                           \
        {                                                                  \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                       \
        }                                                                  \
    } while (0)

constexpr bool USE_VALIDATION_LAYERS = true;

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) { deletors.push_back(std::move(function)); }
    void flush()
    {
        for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) (*it)();
        deletors.clear();
    }
};

struct AllocatedImage
{
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent3D extent;
    VkFormat format;
};

namespace vk_init
{
VkCommandPoolCreateInfo command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1);

VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer command_buffer);

VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* command_buffer_info,
                          VkSemaphoreSubmitInfo* signal_semaphore_info,
                          VkSemaphoreSubmitInfo* wait_semaphore_info);
VkPresentInfoKHR present_info();

VkRenderingAttachmentInfo attachment_info(VkImageView view,
                                          VkClearValue* clear,
                                          VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingAttachmentInfo depth_attachment_info(VkImageView view,
                                                VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingInfo rendering_info(VkExtent2D render_extent,
                               VkRenderingAttachmentInfo* color_attachment,
                               VkRenderingAttachmentInfo* depth_attachment);

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);

VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);
VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type,
                                                          VkShaderStageFlags stage_flags,
                                                          uint32_t binding);
VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info(VkDescriptorSetLayoutBinding* bindings,
                                                                 uint32_t binding_count);
VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type,
                                            VkDescriptorSet set,
                                            VkDescriptorImageInfo* imageInfo,
                                            uint32_t binding);
VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type,
                                             VkDescriptorSet set,
                                             VkDescriptorBufferInfo* bufferInfo,
                                             uint32_t binding);
VkDescriptorBufferInfo buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent);
VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags);
VkPipelineLayoutCreateInfo pipeline_layout_create_info();
VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                                                                  VkShaderModule shader_module,
                                                                  const char* entry = "main");
}  // namespace vk_init

namespace vk_util
{

void transition_image(VkCommandBuffer command_buffer, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout);

void copy_image_to_image(VkCommandBuffer command_bufferr,
                         VkImage source,
                         VkImage destination,
                         VkExtent2D srcSize,
                         VkExtent2D dstSize);

void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
}  // namespace vk_util