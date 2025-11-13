//
// Created by kennypc on 11/4/25.
//

#include <fstream>

void DescriptorAllocator::init_pool(VkDevice device, const uint32_t max_sets, const std::span<PoolSizeRatio> pool_ratios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (const PoolSizeRatio ratio : pool_ratios)
    {
        poolSizes.push_back(
            VkDescriptorPoolSize{.type = ratio.type,
                                 .descriptorCount = static_cast<uint32_t>(ratio.ratio * static_cast<float>(max_sets))});
    }

    VkDescriptorPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = 0;
    pool_info.maxSets = max_sets;
    pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    pool_info.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
}

void DescriptorAllocator::clear_descriptors(VkDevice device) const { vkResetDescriptorPool(device, pool, 0); }

void DescriptorAllocator::destroy_pool(VkDevice device) const { vkDestroyDescriptorPool(device, pool, nullptr); }

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout) const
{
    VkDescriptorSetAllocateInfo allocInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptor_set;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descriptor_set));

    return descriptor_set;
}

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

VkRenderingInfo vk_init::rendering_info(VkExtent2D render_extent,
                                        VkRenderingAttachmentInfo* color_attachment,
                                        VkRenderingAttachmentInfo* depth_attachment)
{
    VkRenderingInfo render_info{};
    render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    render_info.pNext = nullptr;

    render_info.renderArea = VkRect2D{VkOffset2D{0, 0}, render_extent};
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments = color_attachment;
    render_info.pDepthAttachment = depth_attachment;
    render_info.pStencilAttachment = nullptr;

    return render_info;
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

VkRenderingAttachmentInfo vk_init::attachment_info(VkImageView view,
                                                   VkClearValue* clear,
                                                   VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/)
{
    VkRenderingAttachmentInfo color_attachment{};
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment.pNext = nullptr;

    color_attachment.imageView = view;
    color_attachment.imageLayout = layout;
    color_attachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if (clear)
    {
        color_attachment.clearValue = *clear;
    }

    return color_attachment;
}

VkPipelineLayoutCreateInfo vk_init::pipeline_layout_create_info()
{
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
}

VkPipelineShaderStageCreateInfo vk_init::pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                                                                           VkShaderModule shader_module,
                                                                           const char* entry)
{
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.stage = stage;
    info.module = shader_module;
    info.pName = entry;
    return info;
}

VkDescriptorType vk_util::slang_to_vk_descriptor_type(const slang::BindingType type)
{
    switch (type)
    {
        case slang::BindingType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case slang::BindingType::CombinedTextureSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case slang::BindingType::Texture:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case slang::BindingType::MutableTexture:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case slang::BindingType::TypedBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case slang::BindingType::MutableTypedBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case slang::BindingType::RawBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case slang::BindingType::MutableRawBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case slang::BindingType::ConstantBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case slang::BindingType::InputRenderTarget:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case slang::BindingType::InlineUniformData:
            return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
        case slang::BindingType::RayTracingAccelerationStructure:
            return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        default:
            KX_ASSERT_MSG(false, "Unknown binding type");
    }
}

VkShaderStageFlags vk_util::slang_to_vk_stage(const SlangStage stage)
{
    switch (stage)
    {
        case SLANG_STAGE_VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case SLANG_STAGE_FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case SLANG_STAGE_COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case SLANG_STAGE_GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case SLANG_STAGE_RAY_GENERATION:
            return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case SLANG_STAGE_INTERSECTION:
            return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case SLANG_STAGE_ANY_HIT:
            return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case SLANG_STAGE_CLOSEST_HIT:
            return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case SLANG_STAGE_MISS:
            return VK_SHADER_STAGE_MISS_BIT_KHR;
        case SLANG_STAGE_CALLABLE:
            return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
        case SLANG_STAGE_MESH:
            return VK_SHADER_STAGE_MESH_BIT_EXT;
        case SLANG_STAGE_AMPLIFICATION:
            return VK_SHADER_STAGE_TASK_BIT_EXT;
        default:
            return VK_SHADER_STAGE_ALL;
    }
}