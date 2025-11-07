//
// Created by kennypc on 11/6/25.
//

#include "resource/shader.hpp"
#include "pipeline.hpp"

using namespace kynetic;

ComputePipelineBuilder& ComputePipelineBuilder::set_shader(std::shared_ptr<ShaderResource> shader)
{
    m_shader = std::move(shader);
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::set_flags(const VkPipelineCreateFlags flags)
{
    m_flags = flags;
    return *this;
}

VkPipeline ComputePipelineBuilder::build(VkDevice device) const
{
    KX_ASSERT_MSG(m_shader, "Shader must be set before building pipeline");

    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = m_shader->get_module();
    stage_info.pName = "main";

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.layout = m_shader->get_layout();
    pipeline_info.stage = stage_info;
    pipeline_info.flags = m_flags;

    VkPipeline pipeline;
    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));

    return pipeline;
}