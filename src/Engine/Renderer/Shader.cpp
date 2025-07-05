//
// Created by kenny on 5-7-25.
//

#include "Device.h"
#include "Shader.h"

namespace Kynetic {

Shader::Shader(const Device &m_device, const VkShaderStageFlagBits type, const std::string &name,
               const std::string &file_name, const std::string &entry_point)
    : Shader(m_device, type, name, read_file(file_name), entry_point) {}

Shader::Shader(const Device &m_device, const VkShaderStageFlagBits type, const std::string &name,
               const std::vector<char> &code, const std::string &entry_point)
    : m_device(m_device), m_name(name), m_entry_point(entry_point), m_type(type) {
    assert(m_device.get_device());
    assert(!name.empty());
    assert(!code.empty());
    assert(!entry_point.empty());

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    m_device.m_disp.createShaderModule(&createInfo, nullptr, &m_shader_module);
}

Shader::Shader(Shader &&other) noexcept : m_device(other.m_device) {
    m_type = other.m_type;
    m_name = std::move(other.m_name);
    m_entry_point = std::move(other.m_entry_point);
    m_shader_module = std::exchange(other.m_shader_module, nullptr);
}

Shader::~Shader() {
    m_device.m_disp.destroyShaderModule(m_shader_module, nullptr);
}

} // Kynetic