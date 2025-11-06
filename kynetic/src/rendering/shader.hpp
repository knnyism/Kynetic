//
// Created by kennypc on 11/6/25.
//

#pragma once

namespace kynetic
{
class Shader : public Resource
{
    VkShaderModule m_shader_module;
    std::filesystem::file_time_type m_last_modified;

    VkPipelineLayout m_pipeline_layout;
    std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;

    void reflect(slang::IComponentType* linked_program);

public:
    Shader(const std::filesystem::path& path, const std::string& name, const std::string& entry_point_name = "main");
    ~Shader() override;

    [[nodiscard]] const VkShaderModule& get_module() const { return m_shader_module; }
    [[nodiscard]] const VkPipelineLayout& get_layout() const { return m_pipeline_layout; }

    [[nodiscard]] const VkDescriptorSetLayout& get_set_layout(size_t index) const { return m_descriptor_set_layouts[index]; }
};
}  // namespace kynetic
