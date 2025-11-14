//
// Created by kennypc on 11/6/25.
//

#pragma once

namespace kynetic
{

class Shader : public Resource
{
public:
    struct BindingInfo
    {
        uint32_t set;
        uint32_t binding;
        VkDescriptorType type;
        uint32_t count;
        std::string name;
    };

private:
    VkShaderModule m_shader_module;
    std::filesystem::file_time_type m_last_modified;

    VkPipelineLayout m_pipeline_layout;
    std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;

    std::map<std::string, BindingInfo> m_binding_map;

    void reflect(slang::IComponentType* linked_program);

public:
    Shader(const std::filesystem::path& path, const std::string& entry_point_name = "main");
    ~Shader() override;

    [[nodiscard]] const VkShaderModule& get_module() const { return m_shader_module; }
    [[nodiscard]] const VkPipelineLayout& get_layout() const { return m_pipeline_layout; }
    [[nodiscard]] const VkDescriptorSetLayout& get_layout_at(const uint32_t index) const
    {
        return m_descriptor_set_layouts[index];
    }

    [[nodiscard]] const std::map<std::string, BindingInfo>& get_binding_map() const { return m_binding_map; }
};
}  // namespace kynetic
