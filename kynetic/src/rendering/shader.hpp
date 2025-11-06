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

public:
    Shader(const std::filesystem::path& path, const std::string& name, const std::string& entry_point_name = "main");
    ~Shader() override;

    [[nodiscard]] const VkShaderModule& get_module() const { return m_shader_module; }
};
}  // namespace kynetic
