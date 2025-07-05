//
// Created by kenny on 5-7-25.
//

#pragma once

namespace Kynetic {

class Device;
class Shader {
    const Device &m_device;
    std::string m_name;
    std::string m_entry_point;
    VkShaderStageFlagBits m_type;
    VkShaderModule m_shader_module{VK_NULL_HANDLE};
public:
    Shader(const Device &m_device, VkShaderStageFlagBits type, const std::string &name, const std::vector<char> &code,
           const std::string &entry_point = "main");

    Shader(const Device &m_device, VkShaderStageFlagBits type, const std::string &name, const std::string &file_name,
           const std::string &entry_point = "main");

    Shader(const Shader &) = delete;
    Shader(Shader &&) noexcept;

    ~Shader();

    Shader &operator=(const Shader &) = delete;
    Shader &operator=(Shader &&) = delete;

    [[nodiscard]] const std::string &name() const { return m_name; }
    [[nodiscard]] const std::string &entry_point() const { return m_entry_point; }
    [[nodiscard]] VkShaderStageFlagBits type() const { return m_type; }
    [[nodiscard]] VkShaderModule module() const { return m_shader_module; }
};
} // Kynetic
