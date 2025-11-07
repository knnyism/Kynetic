//
// Created by kennypc on 11/6/25.
//

#include "core/device.hpp"
#include "core/engine.hpp"

#include "resource/shader.hpp"

using namespace kynetic;

ShaderResource::ShaderResource(const std::filesystem::path& path, const std::string& entry_point_name)
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

    KX_ASSERT_MSG(module, "Failed to load shader. Path: {}", path.c_str());

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

ShaderResource::~ShaderResource()
{
    Device& device = Engine::get().device();

    if (m_pipeline_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device.get(), m_pipeline_layout, nullptr);

    for (VkDescriptorSetLayout layout : m_descriptor_set_layouts)
        if (layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device.get(), layout, nullptr);

    if (m_shader_module != VK_NULL_HANDLE) vkDestroyShaderModule(device.get(), m_shader_module, nullptr);
}

void ShaderResource::reflect(slang::IComponentType* linked_program)
{
    Device& device = Engine::get().device();
    slang::ProgramLayout* program_layout = linked_program->getLayout();

    VkShaderStageFlags stage_flags = 0;
    SlangUInt entry_point_count = program_layout->getEntryPointCount();

    for (SlangUInt i = 0; i < entry_point_count; ++i)
    {
        slang::EntryPointLayout* entry_point = program_layout->getEntryPointByIndex(i);
        stage_flags |= vk_util::slang_to_vk_stage(entry_point->getStage());
    }

    std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindings_by_set;
    std::vector<VkPushConstantRange> push_constant_ranges;

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

            push_constant_ranges.push_back(range);
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

            bindings_by_set[static_cast<uint32_t>(space)].push_back(vk_binding);

            BindingInfo info;
            info.set = static_cast<uint32_t>(space);
            info.binding = static_cast<uint32_t>(binding);
            info.type = vk_binding.descriptorType;
            info.count = vk_binding.descriptorCount;
            info.name = param->getName();

            m_binding_map[info.name] = info;
        }
    }

    m_descriptor_set_layouts.resize(bindings_by_set.empty() ? 0 : bindings_by_set.rbegin()->first + 1, VK_NULL_HANDLE);

    for (auto& [set_index, bindings] : bindings_by_set)
    {
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();
        VK_CHECK(vkCreateDescriptorSetLayout(device.get(), &layout_info, nullptr, &m_descriptor_set_layouts[set_index]));
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(m_descriptor_set_layouts.size());
    pipeline_layout_info.pSetLayouts = m_descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
    pipeline_layout_info.pPushConstantRanges = push_constant_ranges.empty() ? nullptr : push_constant_ranges.data();

    VK_CHECK(vkCreatePipelineLayout(device.get(), &pipeline_layout_info, nullptr, &m_pipeline_layout));
}
