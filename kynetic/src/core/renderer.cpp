//
// Created by kennypc on 11/5/25.
//

#include "rendering/shader.hpp"

#include "device.hpp"
#include "engine.hpp"
#include "resource_manager.hpp"

#include "renderer.hpp"

using namespace kynetic;

Renderer::Renderer()
{
    Device& device = Engine::get().device();
    m_draw_image = device.create_image(device.get_extent(),
                                       VK_FORMAT_R16G16B16A16_SFLOAT,
                                       VMA_MEMORY_USAGE_GPU_ONLY,
                                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                           VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       VK_IMAGE_ASPECT_COLOR_BIT);

    m_deletion_queue.push_function([this, &device]() { device.destroy_image(m_draw_image); });

    m_gradient_shader = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/gradient.slang", "gradient");
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

    m_descriptor_allocator.init_pool(device.get(), 10, sizes);

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        m_draw_image_descriptor_layout = builder.build(device.get(), VK_SHADER_STAGE_COMPUTE_BIT);
    }

    m_draw_image_descriptors = m_descriptor_allocator.allocate(device.get(), m_draw_image_descriptor_layout);
    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = m_draw_image.view;

    VkWriteDescriptorSet drawImageWrite = {};
    drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    drawImageWrite.pNext = nullptr;

    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = m_draw_image_descriptors;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    drawImageWrite.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(device.get(), 1, &drawImageWrite, 0, nullptr);

    m_deletion_queue.push_function(
        [&]()
        {
            m_descriptor_allocator.destroy_pool(device.get());
            vkDestroyDescriptorSetLayout(device.get(), m_draw_image_descriptor_layout, nullptr);
        });

    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.pSetLayouts = &m_draw_image_descriptor_layout;
    computeLayout.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(device.get(), &computeLayout, nullptr, &m_gradient_pipeline_layout));

    VkPipelineShaderStageCreateInfo stageinfo{};
    stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageinfo.pNext = nullptr;
    stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = m_gradient_shader->get_module();
    stageinfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.layout = m_gradient_pipeline_layout;
    computePipelineCreateInfo.stage = stageinfo;
    VK_CHECK(
        vkCreateComputePipelines(device.get(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_gradient_pipeline));

    m_deletion_queue.push_function(
        [&]()
        {
            vkDestroyPipelineLayout(device.get(), m_gradient_pipeline_layout, nullptr);
            vkDestroyPipeline(device.get(), m_gradient_pipeline, nullptr);
        });
}
Renderer::~Renderer() { m_deletion_queue.flush(); }

void Renderer::render()
{
    Device& device = Engine::get().device();
    const auto& ctx = device.get_context();
    const auto& render_target = device.get_render_target();

    vk_util::transition_image(ctx.dcb, m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    vkCmdBindPipeline(ctx.dcb, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradient_pipeline);

    vkCmdBindDescriptorSets(ctx.dcb,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            m_gradient_pipeline_layout,
                            0,
                            1,
                            &m_draw_image_descriptors,
                            0,
                            nullptr);

    vkCmdDispatch(ctx.dcb,
                  static_cast<uint32_t>(std::ceilf(static_cast<float>(m_draw_image.extent.width) / 16.0f)),
                  static_cast<uint32_t>(std::ceilf(static_cast<float>(m_draw_image.extent.height) / 16.0f)),
                  1);

    vk_util::transition_image(ctx.dcb, m_draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vk_util::transition_image(ctx.dcb, render_target, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vk_util::copy_image_to_image(ctx.dcb,
                                 m_draw_image.image,
                                 render_target,
                                 {.width = m_draw_image.extent.width, .height = m_draw_image.extent.height},
                                 device.get_extent());
    vk_util::transition_image(ctx.dcb, render_target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    m_frame_count++;
}