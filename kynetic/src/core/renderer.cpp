//
// Created by kennypc on 11/5/25.
//

#include "rendering/shader.hpp"
#include "rendering/pipeline.hpp"

#include "device.hpp"
#include "engine.hpp"
#include "resource_manager.hpp"

#include "renderer.hpp"

#include "imgui.h"

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

    m_gradient = Engine::get().resources().load<Shader>("assets/shared_assets/shaders/gradient.slang", "gradient");

    m_gradient_pipeline = ComputePipelineBuilder().set_shader(m_gradient).build(device.get());
    m_deletion_queue.push_function([this, &device]() { vkDestroyPipeline(device.get(), m_gradient_pipeline, nullptr); });

    m_draw_image_descriptor_set = device.get_descriptor_allocator().allocate(device.get(), m_gradient->get_set_layout(0));
    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = m_draw_image.view;

    VkWriteDescriptorSet drawImageWrite = {};
    drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    drawImageWrite.pNext = nullptr;

    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = m_draw_image_descriptor_set;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    drawImageWrite.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(device.get(), 1, &drawImageWrite, 0, nullptr);
}

Renderer::~Renderer() { m_deletion_queue.flush(); }

void Renderer::render()
{
    ImGui::ShowDemoWindow();

    Device& device = Engine::get().device();
    const auto& ctx = device.get_context();
    const auto& render_target = device.get_render_target();

    vk_util::transition_image(ctx.dcb, m_draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    vkCmdBindPipeline(ctx.dcb, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradient_pipeline);
    vkCmdBindDescriptorSets(ctx.dcb,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            m_gradient->get_layout(),
                            0,
                            1,
                            &m_draw_image_descriptor_set,
                            0,
                            nullptr);

    data.data1 = glm::vec4(1, 0, 0, 1);
    data.data2 = glm::vec4(0, 0, 1, 1);

    vkCmdPushConstants(ctx.dcb, m_gradient->get_layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &data);
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

    m_frame_count++;
}