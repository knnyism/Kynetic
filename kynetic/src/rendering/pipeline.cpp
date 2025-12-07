//
// Created by kenny on 11/6/25.
//

#include "shader.hpp"
#include "pipeline.hpp"

#include "core/device.hpp"

#include <ranges>

using namespace kynetic;

Pipeline::Pipeline(Device& device,
                   const std::vector<VkDescriptorSetLayout>& set_layouts,
                   const std::vector<VkPushConstantRange>& push_constant_ranges,
                   VkComputePipelineCreateInfo pipeline_info)
    : m_type(Type::Compute), m_device(device.get()), m_set_layouts(set_layouts)
{
    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = static_cast<uint32_t>(m_set_layouts.size());
    layout_info.pSetLayouts = m_set_layouts.data();
    layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
    layout_info.pPushConstantRanges = push_constant_ranges.data();

    VK_CHECK(vkCreatePipelineLayout(device.get(), &layout_info, nullptr, &m_layout));

    pipeline_info.layout = m_layout;

    VK_CHECK(vkCreateComputePipelines(device.get(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_pipeline));
}

Pipeline::Pipeline(Device& device,
                   const std::vector<VkDescriptorSetLayout>& set_layouts,
                   const std::vector<VkPushConstantRange>& push_constant_ranges,
                   VkGraphicsPipelineCreateInfo pipeline_info)
    : m_type(Type::Graphics), m_device(device.get()), m_set_layouts(set_layouts)
{
    auto set_layouts_with_bindless = set_layouts;

    set_layouts_with_bindless.push_back(device.get_bindless_set_layout());

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = static_cast<uint32_t>(set_layouts_with_bindless.size());
    layout_info.pSetLayouts = set_layouts_with_bindless.data();
    layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
    layout_info.pPushConstantRanges = push_constant_ranges.data();

    VK_CHECK(vkCreatePipelineLayout(device.get(), &layout_info, nullptr, &m_layout));

    pipeline_info.layout = m_layout;

    VK_CHECK(vkCreateGraphicsPipelines(device.get(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_pipeline));
}

Pipeline::~Pipeline()
{
    for (VkDescriptorSetLayout layout : m_set_layouts) vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
    vkDestroyPipelineLayout(m_device, m_layout, nullptr);
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
}

Pipeline::Pipeline(Pipeline&& other) noexcept
    : m_type(other.m_type),
      m_device(other.m_device),
      m_pipeline(other.m_pipeline),
      m_layout(other.m_layout),
      m_set_layouts(std::move(other.m_set_layouts))
{
    other.m_pipeline = VK_NULL_HANDLE;
    other.m_layout = VK_NULL_HANDLE;
}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
{
    if (this != &other)
    {
        for (VkDescriptorSetLayout layout : m_set_layouts) vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
        if (m_pipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_device, m_pipeline, nullptr);
        if (m_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_device, m_layout, nullptr);

        m_type = other.m_type;
        m_device = other.m_device;
        m_pipeline = other.m_pipeline;
        m_layout = other.m_layout;
        m_set_layouts = std::move(other.m_set_layouts);

        other.m_pipeline = VK_NULL_HANDLE;
        other.m_layout = VK_NULL_HANDLE;
    }
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::set_shader(const std::shared_ptr<Shader>& shader)
{
    m_shader = shader;
    return *this;
}

ComputePipelineBuilder& ComputePipelineBuilder::set_flags(const VkPipelineCreateFlags flags)
{
    m_flags = flags;
    return *this;
}

Pipeline ComputePipelineBuilder::build(Device& device) const
{
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = m_shader->get_stages()[0];
    pipeline_info.flags = m_flags;

    const auto& bindings_by_set = m_shader->get_bindings();

    std::vector<VkDescriptorSetLayout> set_layouts;
    set_layouts.resize(bindings_by_set.empty() ? 0 : bindings_by_set.rbegin()->first + 1, VK_NULL_HANDLE);

    for (const auto& [set_index, bindings] : bindings_by_set)
    {
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();
        VK_CHECK(vkCreateDescriptorSetLayout(device.get(), &layout_info, nullptr, &set_layouts[set_index]));
    }

    return Pipeline(device, set_layouts, m_shader->get_push_constant_ranges(), pipeline_info);
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_shader(const std::shared_ptr<Shader>& shader)
{
    m_shader = shader;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_input_topology(const VkPrimitiveTopology topology)
{
    m_input_assembly.topology = topology;
    m_input_assembly.primitiveRestartEnable = VK_FALSE;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_polygon_mode(const VkPolygonMode mode)
{
    m_rasterizer.polygonMode = mode;
    m_rasterizer.lineWidth = 1.f;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    m_rasterizer.cullMode = cullMode;
    m_rasterizer.frontFace = frontFace;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_multisampling_none()
{
    m_multisampling.sampleShadingEnable = VK_FALSE;
    m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    m_multisampling.minSampleShading = 1.0f;
    m_multisampling.pSampleMask = nullptr;
    m_multisampling.alphaToCoverageEnable = VK_FALSE;
    m_multisampling.alphaToOneEnable = VK_FALSE;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::disable_blending()
{
    m_color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    m_color_blend_attachment.blendEnable = VK_FALSE;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_color_attachment_format(VkFormat format)
{
    m_color_attachment_format = format;
    m_render_info.colorAttachmentCount = 1;
    m_render_info.pColorAttachmentFormats = &m_color_attachment_format;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_depth_format(VkFormat format)
{
    m_render_info.depthAttachmentFormat = format;

    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::disable_depthtest()
{
    m_depth_stencil.depthTestEnable = VK_FALSE;
    m_depth_stencil.depthWriteEnable = VK_FALSE;
    m_depth_stencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    m_depth_stencil.depthBoundsTestEnable = VK_FALSE;
    m_depth_stencil.stencilTestEnable = VK_FALSE;
    m_depth_stencil.front = {};
    m_depth_stencil.back = {};
    m_depth_stencil.minDepthBounds = 0.f;
    m_depth_stencil.maxDepthBounds = 1.f;

    return *this;
}
GraphicsPipelineBuilder& GraphicsPipelineBuilder::enable_depthtest(bool depth_write_enable, VkCompareOp op)
{
    m_depth_stencil.depthTestEnable = VK_TRUE;
    m_depth_stencil.depthWriteEnable = depth_write_enable;
    m_depth_stencil.depthCompareOp = op;
    m_depth_stencil.depthBoundsTestEnable = VK_FALSE;
    m_depth_stencil.stencilTestEnable = VK_FALSE;
    m_depth_stencil.front = {};
    m_depth_stencil.back = {};
    m_depth_stencil.minDepthBounds = 0.f;
    m_depth_stencil.maxDepthBounds = 1.f;

    return *this;
}

Pipeline GraphicsPipelineBuilder::build(Device& device) const
{
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = nullptr;

    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.pNext = nullptr;

    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &m_color_blend_attachment;

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {.sType =
                                                                  VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkGraphicsPipelineCreateInfo pipeline_info = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline_info.pNext = &m_render_info;

    const auto& stages = m_shader->get_stages();

    pipeline_info.stageCount = static_cast<uint32_t>(stages.size());
    pipeline_info.pStages = stages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &m_input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &m_rasterizer;
    pipeline_info.pMultisampleState = &m_multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDepthStencilState = &m_depth_stencil;

    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_info = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_info.pDynamicStates = &state[0];
    dynamic_info.dynamicStateCount = 2;

    pipeline_info.pDynamicState = &dynamic_info;

    const auto& bindings_by_set = m_shader->get_bindings();

    std::vector<VkDescriptorSetLayout> set_layouts;

    set_layouts.resize(bindings_by_set.empty() ? 0 : bindings_by_set.rbegin()->first + 1, VK_NULL_HANDLE);

    for (const auto& [set_index, bindings] : bindings_by_set)
    {
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();
        VK_CHECK(vkCreateDescriptorSetLayout(device.get(), &layout_info, nullptr, &set_layouts[set_index]));
    }

    return Pipeline(device, set_layouts, m_shader->get_push_constant_ranges(), pipeline_info);
}