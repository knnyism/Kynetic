//
// Created by kennypc on 11/6/25.
//

#pragma once

namespace kynetic
{

class Shader;

class Pipeline
{
    enum class Type
    {
        Compute,
        Graphics,
    } m_type;

    VkDevice m_device{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_layout{VK_NULL_HANDLE};

public:
    Pipeline(VkDevice device, const VkComputePipelineCreateInfo& pipeline_info);
    Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& pipeline_info);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&& other) noexcept;
    Pipeline& operator=(Pipeline&& other) noexcept;

    [[nodiscard]] const VkPipeline& get() const { return m_pipeline; }
    [[nodiscard]] const VkPipelineLayout& get_layout() const { return m_layout; }

    [[nodiscard]] VkPipelineBindPoint bind_point() const
    {
        switch (m_type)
        {
            case Type::Graphics:
                return VK_PIPELINE_BIND_POINT_GRAPHICS;
            case Type::Compute:
                return VK_PIPELINE_BIND_POINT_COMPUTE;
            default:
                KX_ASSERT(false);
        }
    }
};

class ComputePipelineBuilder
{
    VkPipelineShaderStageCreateInfo m_stage{};
    VkPipelineLayout m_layout{VK_NULL_HANDLE};
    VkPipelineCreateFlags m_flags{0};

public:
    ComputePipelineBuilder& set_shader(const std::shared_ptr<Shader>& shader);
    ComputePipelineBuilder& set_flags(VkPipelineCreateFlags flags);

    Pipeline build(VkDevice device) const;
};

class GraphicsPipelineBuilder
{
    std::vector<VkPipelineShaderStageCreateInfo> m_stages;
    std::vector<VkPipelineLayout> m_layouts;
    VkPipelineCreateFlags m_flags{0};

    VkPipelineInputAssemblyStateCreateInfo m_input_assembly{.sType =
                                                                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    VkPipelineRasterizationStateCreateInfo m_rasterizer{.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState m_color_blend_attachment{};
    VkPipelineMultisampleStateCreateInfo m_multisampling{.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    VkPipelineLayout m_pipeline_layout{};
    VkPipelineDepthStencilStateCreateInfo m_depth_stencil{.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineRenderingCreateInfo m_render_info{.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    VkFormat m_color_attachment_format{};

public:
    GraphicsPipelineBuilder& set_shader(const std::shared_ptr<Shader>& vert, const std::shared_ptr<Shader>& frag);
    GraphicsPipelineBuilder& set_input_topology(VkPrimitiveTopology topology);
    GraphicsPipelineBuilder& set_polygon_mode(VkPolygonMode mode);
    GraphicsPipelineBuilder& set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    GraphicsPipelineBuilder& set_multisampling_none();
    GraphicsPipelineBuilder& disable_blending();
    GraphicsPipelineBuilder& set_color_attachment_format(VkFormat format);
    GraphicsPipelineBuilder& set_depth_format(VkFormat format);
    GraphicsPipelineBuilder& disable_depthtest();
    GraphicsPipelineBuilder& enable_depthtest(bool depth_write_enable, VkCompareOp op);

    // GraphicsPipelineBuilder& set_flags(VkPipelineCreateFlags flags);

    Pipeline build(VkDevice device) const;
};

}  // namespace kynetic