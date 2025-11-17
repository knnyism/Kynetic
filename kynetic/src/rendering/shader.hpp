//
// Created by kenny on 11/6/25.
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

    std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> m_bindings_by_set;
    std::vector<VkPushConstantRange> m_push_constant_ranges;

    std::map<std::string, BindingInfo> m_binding_map;

    void reflect(slang::IComponentType* linked_program);

public:
    Shader(const std::filesystem::path& path, const std::string& entry_point_name = "main");
    ~Shader() override;

    [[nodiscard]] const VkShaderModule& get_module() const { return m_shader_module; }

    [[nodiscard]] const std::vector<VkPushConstantRange>& get_push_constant_ranges() const { return m_push_constant_ranges; }
    [[nodiscard]] const std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& get_bindings()
    {
        return m_bindings_by_set;
    }
    [[nodiscard]] const std::map<std::string, BindingInfo>& get_binding_map() const { return m_binding_map; }
};
}  // namespace kynetic
