//
// Created by kennypc on 11/6/25.
//

#pragma once

namespace kynetic
{
class ComputePipelineBuilder
{
    std::shared_ptr<class ShaderResource> m_shader;
    VkPipelineCreateFlags m_flags = 0;

public:
    ComputePipelineBuilder& set_shader(std::shared_ptr<ShaderResource> shader);

    ComputePipelineBuilder& set_flags(VkPipelineCreateFlags flags);

    VkPipeline build(VkDevice device) const;
};
}  // namespace kynetic