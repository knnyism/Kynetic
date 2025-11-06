//
// Created by kennypc on 11/6/25.
//

#include "core/device.hpp"
#include "core/engine.hpp"

#include "shader.hpp"

using namespace kynetic;

Shader::Shader(const std::filesystem::path& path, const std::string& name, const std::string& entry_point_name)
    : Resource(Type::Shader, path)
{
    Device& device = Engine::get().device();

    slang::SessionDesc session_desc = {};
    slang::TargetDesc target_desc = {};
    target_desc.format = SLANG_SPIRV;
    target_desc.profile = device.get_slang_session()->findProfile("spirv_1_5");

    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;

    std::array<slang::CompilerOptionEntry, 1> options = {
        {{
            slang::CompilerOptionName::EmitSpirvDirectly,
            {slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr},
        }},
    };
    session_desc.compilerOptionEntries = options.data();
    session_desc.compilerOptionEntryCount = options.size();

    Slang::ComPtr<slang::ISession> session;
    device.get_slang_session()->createSession(session_desc, session.writeRef());

    Slang::ComPtr<slang::IModule> module;
    Slang::ComPtr<slang::IBlob> diagnostics;
    module = session->loadModule(path.c_str(), diagnostics.writeRef());
    DIAGNOSE(diagnostics);

    KX_ASSERT_MSG(module, "Failed to load shader \"{}\". Path: {}", name, path.c_str());

    Slang::ComPtr<slang::IEntryPoint> entry_point;
    SlangResult result = module->findEntryPointByName(entry_point_name.c_str(), entry_point.writeRef());
    KX_ASSERT_MSG(result == SLANG_OK, "Failed get entry point \"{}\". Path: {}", entry_point_name, path.c_str());

    const std::array<slang::IComponentType*, 2> component_types = {module, entry_point};

    Slang::ComPtr<slang::IComponentType> composed_program;
    result = session->createCompositeComponentType(component_types.data(),
                                                   component_types.size(),
                                                   composed_program.writeRef(),
                                                   diagnostics.writeRef());
    DIAGNOSE(diagnostics);
    KX_ASSERT(result == SLANG_OK);

    Slang::ComPtr<slang::IComponentType> linked_program;
    result = composed_program->link(linked_program.writeRef(), diagnostics.writeRef());
    DIAGNOSE(diagnostics);
    KX_ASSERT(result == SLANG_OK);

    Slang::ComPtr<slang::IBlob> spirv_code;
    result = linked_program->getTargetCode(0,  // targetIndex
                                           spirv_code.writeRef(),
                                           diagnostics.writeRef());
    DIAGNOSE(diagnostics);
    KX_ASSERT(result == SLANG_OK);

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.codeSize = spirv_code->getBufferSize();
    create_info.pCode = static_cast<const uint32_t*>(spirv_code->getBufferPointer());

    result = vkCreateShaderModule(device.get(), &create_info, nullptr, &m_shader_module);
    KX_ASSERT(result == VK_SUCCESS);
}

Shader::~Shader() { vkDestroyShaderModule(Engine::get().device().get(), m_shader_module, nullptr); }
