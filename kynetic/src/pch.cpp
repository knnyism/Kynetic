//
// Created by kennypc on 11/4/25.
//

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VkCommandPoolCreateInfo vk_init::command_pool_create_info(const uint32_t queue_family_index,
                                                          const VkCommandPoolCreateFlags flags)
{
    return {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
            .queueFamilyIndex = queue_family_index};
}
VkCommandBufferAllocateInfo vk_init::command_buffer_allocate_info(VkCommandPool pool, const uint32_t count)
{
    return {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = count};
}

VkFenceCreateInfo vk_init::fence_create_info(const VkFenceCreateFlags flags /*= 0*/)
{
    return {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = flags};
}

VkSemaphoreCreateInfo vk_init::semaphore_create_info(const VkSemaphoreCreateFlags flags /*= 0*/)
{
    return {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = flags};
}
VkImageSubresourceRange vk_init::image_subresource_range(VkImageAspectFlags aspect_mask)
{
    return {.aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS};
}

VkCommandBufferBeginInfo vk_init::command_buffer_begin_info(const VkCommandBufferUsageFlags flags /*= 0*/)
{
    return {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = flags,
            .pInheritanceInfo = nullptr};
}

VkSemaphoreSubmitInfo vk_init::semaphore_submit_info(const VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore)
{
    return {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext = nullptr,
            .semaphore = semaphore,
            .value = 1,
            .stageMask = stage_mask,
            .deviceIndex = 0};
}

VkCommandBufferSubmitInfo vk_init::command_buffer_submit_info(VkCommandBuffer command_buffer)
{
    return {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = command_buffer,
            .deviceMask = 0};
}

VkSubmitInfo2 vk_init::submit_info(VkCommandBufferSubmitInfo* command_buffer_info,
                                   VkSemaphoreSubmitInfo* signal_semaphore_info,
                                   VkSemaphoreSubmitInfo* wait_semaphore_info)
{
    return {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .pNext = nullptr,
            .waitSemaphoreInfoCount = static_cast<uint32_t>(wait_semaphore_info == nullptr ? 0 : 1),
            .pWaitSemaphoreInfos = wait_semaphore_info,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = command_buffer_info,
            .signalSemaphoreInfoCount = static_cast<uint32_t>(signal_semaphore_info == nullptr ? 0 : 1),
            .pSignalSemaphoreInfos = signal_semaphore_info};
}

VkImageCreateInfo vk_init::image_create_info(const VkFormat format,
                                             const VkImageUsageFlags usage_flags,
                                             const VkExtent3D extent)
{
    return {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,

            .imageType = VK_IMAGE_TYPE_2D,

            .format = format,
            .extent = extent,

            .mipLevels = 1,
            .arrayLayers = 1,

            .samples = VK_SAMPLE_COUNT_1_BIT,

            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage_flags};
}

VkImageViewCreateInfo vk_init::imageview_create_info(const VkFormat format,
                                                     VkImage image,
                                                     const VkImageAspectFlags aspect_flags)
{
    return {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {
                .aspectMask = aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }};
}

void vk_util::transition_image(VkCommandBuffer command_buffer,
                               VkImage image,
                               VkImageLayout current_layout,
                               VkImageLayout new_layout)
{
    const VkImageAspectFlags aspect_mask =
        (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageMemoryBarrier2 image_barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                        .pNext = nullptr,

                                        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                                        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,

                                        .oldLayout = current_layout,
                                        .newLayout = new_layout,
                                        .image = image,
                                        .subresourceRange = vk_init::image_subresource_range(aspect_mask)};

    const VkDependencyInfo dependency_info{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                           .pNext = nullptr,
                                           .imageMemoryBarrierCount = 1,
                                           .pImageMemoryBarriers = &image_barrier};

    vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

void vk_util::copy_image_to_image(VkCommandBuffer command_bufferr,
                                  VkImage source,
                                  VkImage destination,
                                  const VkExtent2D srcSize,
                                  const VkExtent2D dstSize)
{
    VkImageBlit2 blit_region{.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr};

    blit_region.srcOffsets[1].x = static_cast<int32_t>(srcSize.width);
    blit_region.srcOffsets[1].y = static_cast<int32_t>(srcSize.height);
    blit_region.srcOffsets[1].z = 1;

    blit_region.dstOffsets[1].x = static_cast<int32_t>(dstSize.width);
    blit_region.dstOffsets[1].y = static_cast<int32_t>(dstSize.height);
    blit_region.dstOffsets[1].z = 1;

    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcSubresource.mipLevel = 0;

    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr};
    blitInfo.dstImage = destination;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage = source;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blit_region;

    vkCmdBlitImage2(command_bufferr, &blitInfo);
}
