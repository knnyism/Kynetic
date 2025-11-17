//
// Created by kenny on 11/6/25.
//

#include "core/device.hpp"
#include "core/engine.hpp"

#include "shader.hpp"

using namespace kynetic;

Shader::Shader(const std::filesystem::path& path, const std::string& entry_point_name) : Resource(Type::Shader, path.string())
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
    session_desc.compilerOptionEntryCount = static_cast<uint32_t>(options.size());

    Slang::ComPtr<slang::ISession> session;
    device.get_slang_session()->createSession(session_desc, session.writeRef());

    Slang::ComPtr<slang::IModule> module;
    Slang::ComPtr<slang::IBlob> diagnostics;
    module = session->loadModule(path.string().c_str(), diagnostics.writeRef());
    DIAGNOSE(diagnostics);

    KX_ASSERT_MSG(module, "Failed to load shader. Path: {}", path.string());

    Slang::ComPtr<slang::IEntryPoint> entry_point;
    SlangResult result = module->findEntryPointByName(entry_point_name.c_str(), entry_point.writeRef());
    KX_ASSERT_MSG(result == SLANG_OK, "Failed get entry point \"{}\". Path: {}", entry_point_name, path.string());

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
    result = linked_program->getTargetCode(0, spirv_code.writeRef(), diagnostics.writeRef());
    DIAGNOSE(diagnostics);
    KX_ASSERT(result == SLANG_OK);

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.codeSize = spirv_code->getBufferSize();
    create_info.pCode = static_cast<const uint32_t*>(spirv_code->getBufferPointer());

    result = vkCreateShaderModule(device.get(), &create_info, nullptr, &m_shader_module);
    KX_ASSERT(result == VK_SUCCESS);

    reflect(linked_program);
}

Shader::~Shader()
{
    Device& device = Engine::get().device();

    if (m_shader_module != VK_NULL_HANDLE) vkDestroyShaderModule(device.get(), m_shader_module, nullptr);
}

void Shader::reflect(slang::IComponentType* linked_program)
{
    slang::ProgramLayout* program_layout = linked_program->getLayout();

    VkShaderStageFlags stage_flags = 0;
    SlangUInt entry_point_count = program_layout->getEntryPointCount();

    for (SlangUInt i = 0; i < entry_point_count; ++i)
    {
        slang::EntryPointLayout* entry_point = program_layout->getEntryPointByIndex(i);
        stage_flags |= vk_util::slang_to_vk_stage(entry_point->getStage());
    }

    SlangUInt param_count = program_layout->getParameterCount();
    for (SlangUInt i = 0; i < param_count; ++i)
    {
        slang::VariableLayoutReflection* param = program_layout->getParameterByIndex(static_cast<unsigned>(i));
        slang::TypeLayoutReflection* type_layout = param->getTypeLayout();

        if (type_layout->getKind() == slang::TypeReflection::Kind::ConstantBuffer &&
            param->getCategory() == slang::ParameterCategory::PushConstantBuffer)
        {
            slang::TypeLayoutReflection* element_type = type_layout->getElementTypeLayout();
            VkPushConstantRange range{};
            range.offset = static_cast<uint32_t>(param->getOffset(slang::ParameterCategory::PushConstantBuffer));
            range.size = static_cast<uint32_t>(element_type->getSize());
            range.stageFlags = stage_flags;

            m_push_constant_ranges.push_back(range);
            continue;
        }

        SlangInt space = param->getBindingSpace();
        SlangInt binding = param->getBindingIndex();

        if (space >= 0 && binding >= 0)
        {
            VkDescriptorSetLayoutBinding vk_binding{};
            vk_binding.binding = static_cast<uint32_t>(binding);
            vk_binding.descriptorType = vk_util::slang_to_vk_descriptor_type(type_layout->getBindingRangeType(0));
            vk_binding.descriptorCount = 1;
            vk_binding.stageFlags = stage_flags;
            vk_binding.pImmutableSamplers = nullptr;

            if (type_layout->getKind() == slang::TypeReflection::Kind::Array)
            {
                vk_binding.descriptorCount = static_cast<uint32_t>(type_layout->getElementCount());
            }

            m_bindings_by_set[static_cast<uint32_t>(space)].push_back(vk_binding);

            BindingInfo info;
            info.set = static_cast<uint32_t>(space);
            info.binding = static_cast<uint32_t>(binding);
            info.type = vk_binding.descriptorType;
            info.count = vk_binding.descriptorCount;
            info.name = param->getName();

            m_binding_map[info.name] = info;
        }
    }
}
