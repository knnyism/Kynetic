//
// Created by kenny on 11/7/25.
//

#include "pipeline.hpp"
#include "command_buffer.hpp"

using namespace kynetic;

void CommandBuffer::init(VkDevice device, uint32_t queue_family_index)
{
    m_device = device;

    VkCommandPoolCreateInfo command_pool_info =
        vk_init::command_pool_create_info(queue_family_index, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VK_CHECK(vkCreateCommandPool(device, &command_pool_info, nullptr, &m_command_pool));

    const VkCommandBufferAllocateInfo command_buffer_allocate_info = vk_init::command_buffer_allocate_info(m_command_pool, 1);

    VK_CHECK(vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &m_command_buffer));
}

void CommandBuffer::shutdown() const { vkDestroyCommandPool(m_device, m_command_pool, nullptr); }

void CommandBuffer::reset()
{
    m_current_pipeline = nullptr;
    VK_CHECK(vkResetCommandBuffer(m_command_buffer, 0));
}

void CommandBuffer::transition_image(VkImage image, VkImageLayout current_layout, VkImageLayout new_layout) const
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

    vkCmdPipelineBarrier2(m_command_buffer, &dependency_info);
}

void CommandBuffer::copy_image_to_image(VkImage source,
                                        VkImage destination,
                                        const VkExtent2D srcSize,
                                        const VkExtent2D dstSize) const
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

    vkCmdBlitImage2(m_command_buffer, &blitInfo);
}
void CommandBuffer::copy_buffer_to_image(VkBuffer srcBuffer,
                                         VkImage dstImage,
                                         VkImageLayout dstImageLayout,
                                         uint32_t regionCount,
                                         const VkBufferImageCopy* pRegions) const
{
    vkCmdCopyBufferToImage(m_command_buffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}
void CommandBuffer::copy_buffer(VkBuffer srcBuffer,
                                VkBuffer dstBuffer,
                                uint32_t regionCount,
                                const VkBufferCopy* pRegions) const
{
    vkCmdCopyBuffer(m_command_buffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void CommandBuffer::begin_rendering(const VkRenderingInfo& rendering_info) const
{
    vkCmdBeginRendering(m_command_buffer, &rendering_info);
}
void CommandBuffer::end_rendering() const { vkCmdEndRendering(m_command_buffer); }

void CommandBuffer::set_viewport(float width, float height) const
{
    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);
}

void CommandBuffer::set_scissor(uint32_t width, uint32_t height) const
{
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = width;
    scissor.extent.height = height;

    vkCmdSetScissor(m_command_buffer, 0, 1, &scissor);
}

void CommandBuffer::bind_pipeline(Pipeline* pipeline)
{
    m_current_pipeline = pipeline;

    vkCmdBindPipeline(m_command_buffer, m_current_pipeline->bind_point(), m_current_pipeline->get());
}

void CommandBuffer::set_push_constants(VkShaderStageFlags stage_flags, uint32_t size, const void* data, uint32_t offset) const
{
    vkCmdPushConstants(m_command_buffer, m_current_pipeline->get_layout(), stage_flags, offset, size, data);
}

// TODO: Use builder pattern and move to pipeline.cpp
void CommandBuffer::pipeline_barrier(VkPipelineStageFlags src_stage_mask,
                                     VkPipelineStageFlags dst_stage_mask,
                                     VkDependencyFlags dependency_flags,
                                     uint32_t memory_barrier_count,
                                     const VkMemoryBarrier* pMemoryBarriers,
                                     uint32_t buffer_memory_barrier_count,
                                     const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                     uint32_t image_memory_barrier_count,
                                     const VkImageMemoryBarrier* pImageMemoryBarriers) const
{
    vkCmdPipelineBarrier(m_command_buffer,
                         src_stage_mask,
                         dst_stage_mask,
                         dependency_flags,
                         memory_barrier_count,
                         pMemoryBarriers,
                         buffer_memory_barrier_count,
                         pBufferMemoryBarriers,
                         image_memory_barrier_count,
                         pImageMemoryBarriers);
}

void CommandBuffer::bind_descriptors(VkDescriptorSet descriptor_set, uint32_t first_set, uint32_t count) const
{
    vkCmdBindDescriptorSets(m_command_buffer,
                            m_current_pipeline->bind_point(),
                            m_current_pipeline->get_layout(),
                            first_set,
                            count,
                            &descriptor_set,
                            0,
                            nullptr);
}

void CommandBuffer::bind_vertex_buffer(uint32_t first_binding,
                                       uint32_t binding_count,
                                       VkBuffer vertex_buffer,
                                       VkDeviceSize offset) const
{
    vkCmdBindVertexBuffers(m_command_buffer, first_binding, binding_count, &vertex_buffer, &offset);
}

void CommandBuffer::bind_index_buffer(VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset) const
{
    vkCmdBindIndexBuffer(m_command_buffer, index_buffer, offset, index_type);
}

void CommandBuffer::dispatch(uint32_t x, uint32_t y, uint32_t z) const { vkCmdDispatch(m_command_buffer, x, y, z); }

void CommandBuffer::draw_auto(uint32_t vertex_count,
                              uint32_t instance_count,
                              uint32_t first_vertex,
                              uint32_t first_instance) const
{
    vkCmdDraw(m_command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::draw(uint32_t index_count,
                         uint32_t instance_count,
                         uint32_t first_index,
                         int32_t vertex_offset,
                         uint32_t first_instance) const
{
    vkCmdDrawIndexed(m_command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::multi_draw_indirect(VkBuffer buffer, uint32_t draw_count, uint32_t stride, VkDeviceSize offset) const
{
    vkCmdDrawIndexedIndirect(m_command_buffer, buffer, offset, draw_count, stride);
}