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
    std::filesystem::file_time_type m_last_modified;

    std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> m_bindings;
    std::vector<VkPushConstantRange> m_push_constant_ranges;

    std::vector<VkShaderModule> m_modules;
    std::vector<VkPipelineShaderStageCreateInfo> m_stages;

    void reflect(slang::IComponentType* linked_program);

public:
    Shader(const std::filesystem::path& path);
    ~Shader() override;

    [[nodiscard]] const std::vector<VkPushConstantRange>& get_push_constant_ranges() const { return m_push_constant_ranges; }
    [[nodiscard]] const std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>>& get_bindings() { return m_bindings; }
    [[nodiscard]] const std::vector<VkPipelineShaderStageCreateInfo>& get_stages() const { return m_stages; }
};
}  // namespace kynetic
