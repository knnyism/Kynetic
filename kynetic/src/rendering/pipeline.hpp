//
// Created by kenny on 11/6/25.
//

#pragma once

namespace kynetic
{

class Device;
class Shader;

class Pipeline
{
    struct BindingInfo
    {
        uint32_t set;
        uint32_t binding;
        VkDescriptorType type;
        uint32_t count;
        std::string name;
    };

    enum class Type
    {
        Compute,
        Graphics,
    } m_type;

    VkDevice m_device{VK_NULL_HANDLE};
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    VkPipelineLayout m_layout{VK_NULL_HANDLE};

    std::vector<VkDescriptorSetLayout> m_set_layouts;

public:
    Pipeline(Device& device,
             const std::vector<VkDescriptorSetLayout>& set_layouts,
             const std::vector<VkPushConstantRange>& push_constant_ranges,
             VkComputePipelineCreateInfo pipeline_info);
    Pipeline(Device& device,
             const std::vector<VkDescriptorSetLayout>& set_layouts,
             const std::vector<VkPushConstantRange>& push_constant_ranges,
             VkGraphicsPipelineCreateInfo pipeline_info);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&& other) noexcept;
    Pipeline& operator=(Pipeline&& other) noexcept;

    [[nodiscard]] const VkPipeline& get() const { return m_pipeline; }
    [[nodiscard]] const VkPipelineLayout& get_layout() const { return m_layout; }

    [[nodiscard]] const VkDescriptorSetLayout& get_set_layout(uint32_t set_index) const { return m_set_layouts[set_index]; }

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
    std::shared_ptr<Shader> m_shader;
    VkPipelineCreateFlags m_flags{0};

public:
    ComputePipelineBuilder& set_shader(const std::shared_ptr<Shader>& shader);
    ComputePipelineBuilder& set_flags(VkPipelineCreateFlags flags);

    Pipeline build(Device& device) const;
};

class GraphicsPipelineBuilder
{
    std::shared_ptr<Shader> m_shader;
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
    GraphicsPipelineBuilder& set_shader(const std::shared_ptr<Shader>& shader);
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

    Pipeline build(Device& device) const;
};

}  // namespace kynetic