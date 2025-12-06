//
// Created by kenny on 11/6/25.
//

#include "core/device.hpp"
#include "core/engine.hpp"

#include "shader.hpp"

using namespace kynetic;

Shader::Shader(const std::filesystem::path& path) : Resource(Type::Shader, path.string())
{
    Device& device = Engine::get().device();

    slang::SessionDesc session_desc = {};
    slang::TargetDesc target_desc = {};
    target_desc.format = SLANG_SPIRV;
    target_desc.profile = device.get_slang_session()->findProfile("SPV_EXT_descriptor_indexing");

    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;

    const char* search_paths[] = {
        "kynetic/src",
        "assets/shaders"
    };
    session_desc.searchPaths = search_paths;
    session_desc.searchPathCount = 2;

    slang::CompilerOptionEntry options[] = {
        {slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}},
        {slang::CompilerOptionName::DebugInformation, {slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_STANDARD}}};
    session_desc.compilerOptionEntries = options;
    session_desc.compilerOptionEntryCount = 2;

    Slang::ComPtr<slang::ISession> session;
    device.get_slang_session()->createSession(session_desc, session.writeRef());

    Slang::ComPtr<slang::IModule> module;
    Slang::ComPtr<slang::IBlob> diagnostics;
    module = session->loadModule(path.string().c_str(), diagnostics.writeRef());
    DIAGNOSE(diagnostics);
    KX_ASSERT_MSG(module, "Failed to load shader. Path: {}", path.string());

    auto entry_point_count = static_cast<size_t>(module->getDefinedEntryPointCount());

    std::vector<slang::IComponentType*> component_types;
    component_types.push_back(module);

    for (size_t i = 0; i < entry_point_count; ++i)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        SlangResult result = module->getDefinedEntryPoint(static_cast<SlangInt32>(i), entry_point.writeRef());
        DIAGNOSE(diagnostics);
        KX_ASSERT(result == SLANG_OK);

        component_types.push_back(entry_point);
    }

    Slang::ComPtr<slang::IComponentType> composed_program;
    SlangResult result = session->createCompositeComponentType(component_types.data(),
                                                               static_cast<SlangInt>(component_types.size()),
                                                               composed_program.writeRef(),
                                                               diagnostics.writeRef());
    DIAGNOSE(diagnostics);
    KX_ASSERT(result == SLANG_OK);

    Slang::ComPtr<slang::IComponentType> linked_program;
    result = composed_program->link(linked_program.writeRef(), diagnostics.writeRef());
    DIAGNOSE(diagnostics);
    KX_ASSERT(result == SLANG_OK);

    reflect(linked_program);

    m_modules.resize(entry_point_count);
    m_stages.resize(entry_point_count);

    for (size_t i = 0; i < entry_point_count; ++i)
    {
        Slang::ComPtr<slang::IBlob> spirv_code;
        result =
            linked_program->getEntryPointCode(static_cast<SlangInt32>(i), 0, spirv_code.writeRef(), diagnostics.writeRef());
        DIAGNOSE(diagnostics);
        KX_ASSERT(result == SLANG_OK);

        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.pNext = nullptr;
        create_info.codeSize = spirv_code->getBufferSize();
        create_info.pCode = static_cast<const uint32_t*>(spirv_code->getBufferPointer());

        result = vkCreateShaderModule(device.get(), &create_info, nullptr, &m_modules[i]);
        KX_ASSERT(result == VK_SUCCESS);

        m_stages[i] = vk_init::pipeline_shader_stage_create_info(
            vk_util::slang_to_vk_stage_bit(linked_program->getLayout()->getEntryPointByIndex(i)->getStage()),
            m_modules[i]);
    }
}

Shader::~Shader()
{
    Device& device = Engine::get().device();
    for (const auto& module : m_modules) vkDestroyShaderModule(device.get(), module, nullptr);
}

void Shader::reflect(slang::IComponentType* linked_program)
{
    slang::ProgramLayout* program_layout = linked_program->getLayout();

    std::unordered_map<const slang::VariableLayoutReflection*, VkShaderStageFlags> param_stage_usage;

    SlangUInt entry_point_count = program_layout->getEntryPointCount();
    for (SlangUInt ep_idx = 0; ep_idx < entry_point_count; ++ep_idx)
    {
        slang::EntryPointLayout* entry_point = program_layout->getEntryPointByIndex(ep_idx);
        VkShaderStageFlags stage_flag = vk_util::slang_to_vk_stage_bit(entry_point->getStage());

        SlangUInt ep_param_count = entry_point->getParameterCount();
        for (SlangUInt i = 0; i < ep_param_count; ++i)
        {
            slang::VariableLayoutReflection* param = entry_point->getParameterByIndex(static_cast<unsigned>(i));
            param_stage_usage[param] |= stage_flag;
        }
    }

    SlangUInt param_count = program_layout->getParameterCount();
    for (SlangUInt i = 0; i < param_count; ++i)
    {
        slang::VariableLayoutReflection* param = program_layout->getParameterByIndex(static_cast<unsigned>(i));
        slang::TypeLayoutReflection* type_layout = param->getTypeLayout();

        VkShaderStageFlags stage_flags = 0;

        auto usage_it = param_stage_usage.find(param);
        if (usage_it != param_stage_usage.end())
        {
            stage_flags = usage_it->second;
        }
        else
        {
            for (SlangUInt ep_idx = 0; ep_idx < entry_point_count; ++ep_idx)
            {
                slang::EntryPointLayout* entry_point = program_layout->getEntryPointByIndex(ep_idx);
                stage_flags |= vk_util::slang_to_vk_stage(entry_point->getStage());
            }
        }

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

        bool is_bindless_binding = space == 1 && binding == 0;

        if (space >= 0 && binding >= 0 && !is_bindless_binding)
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

            m_bindings[static_cast<uint32_t>(space)].push_back(vk_binding);
        }
    }
}
