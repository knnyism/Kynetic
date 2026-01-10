//
// Created by kenny on 11/4/25.
//

#pragma once

#define NOMINMAX

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>
#include <thread>
#include <filesystem>
#include <map>
#include <ranges>
#include <algorithm>

#include "flecs.h"
#include "fmt/core.h"

#include "glm/mat4x4.hpp"
#include "glm/vec4.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/transform.hpp"

#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/glm_element_traits.hpp"

#include "slang.h"
#include "slang-com-ptr.h"
#include "slang-com-helper.h"

#include "volk.h"
#include "vulkan/vk_enum_string_helper.h"
#include "vma_usage.hpp"

#include "imgui.h"
#include "magic_enum/magic_enum.hpp"

#include "stb_image.h"

#include "shader_types.hpp"

#define VK_CHECK(x)                                                                                             \
    do                                                                                                          \
    {                                                                                                           \
        VkResult err = x;                                                                                       \
        if (err)                                                                                                \
        {                                                                                                       \
            fmt::print(stderr, "Detected Vulkan error: {} at {}:{}", string_VkResult(err), __FILE__, __LINE__); \
            abort();                                                                                            \
        }                                                                                                       \
    } while (0)

#define VMA_LEAK_LOG_FORMAT(format, ...) \
    {                                    \
        printf((format), __VA_ARGS__);   \
        printf("\n");                    \
    }

#define DIAGNOSE(diagnostics) \
    if (diagnostics) printf("%s", (const char*)diagnostics->getBufferPointer());

#define KX_ASSERT(condition)                                                                       \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            fmt::print(stderr, "Assertion failed: {} at {}:{}\n", #condition, __FILE__, __LINE__); \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

#define KX_ASSERT_MSG(condition, ...)                                                              \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            fmt::print(stderr, "Assertion failed: {} at {}:{}\n", #condition, __FILE__, __LINE__); \
            fmt::print(stderr, __VA_ARGS__);                                                       \
            fmt::print(stderr, "\n");                                                              \
            std::abort();                                                                          \
        }                                                                                          \
    } while (0)

// clang-format off
// NOLINTBEGIN
#if defined(__clang__) || defined(__GNUC__)
#define KX_DO_PRAGMA(X) _Pragma(#X)
#define KX_DISABLE_WARNING_PUSH KX_DO_PRAGMA(GCC diagnostic push)
#define KX_DISABLE_WARNING_POP KX_DO_PRAGMA(GCC diagnostic pop)
#define KX_DISABLE_WARNING(warningName) KX_DO_PRAGMA(GCC diagnostic ignored #warningName)
#define KX_DISABLE_WARNING_DEPRECATED_DECLARATIONS KX_DISABLE_WARNING(-Wdeprecated-declarations)
#define KX_DISABLE_WARNING_HIDES_LOCAL_DECLARATION KX_DISABLE_WARNING(-Wshadow)
#define KX_DISABLE_WARNING_MISSING_FIELD_INITIALIZERS KX_DISABLE_WARNING(-Wmissing-field-initializers)
#define KX_DISABLE_WARNING_NAMELESS_STRUCT
#define KX_DISABLE_WARNING_SIGNED_UNSIGNED_COMPARISON_MISMATCH KX_DISABLE_WARNING(-Wsign-conversion)
#define KX_DISABLE_WARNING_SIGNED_UNSIGNED_ASSIGNMENT_MISMATCH KX_DISABLE_WARNING(-Wsign-conversion)
#define KX_DISABLE_WARNING_CONVERSION KX_DISABLE_WARNING(-Wconversion)
#define KX_DISABLE_WARNING_UNREFERENCED_LOCAL_VARIABLE KX_DISABLE_WARNING(-Wunused-variable)
#define KX_DISABLE_WARNING_UNUSED_PARAMETER KX_DISABLE_WARNING(-Wunused-parameter)
#define KX_DISABLE_WARNING_UNUSED_FUNCTION KX_DISABLE_WARNING(-Wunused-function)
#define KX_DISABLE_WARNING_UNUSED_VARIABLE KX_DISABLE_WARNING(-Wunused-variable)
#define KX_DISABLE_WARNING_OUTSIDE_RANGE KX_DISABLE_WARNING(-Wconversion)
#else
#define KX_DISABLE_WARNING_PUSH __pragma(warning(push))
#define KX_DISABLE_WARNING_POP __pragma(warning(pop))
#define KX_DISABLE_WARNING(warningNumber) __pragma(warning(disable : warningNumber))
#define KX_DISABLE_WARNING_DEPRECATED_DECLARATIONS KX_DISABLE_WARNING(4996)
#define KX_DISABLE_WARNING_HIDES_LOCAL_DECLARATION KX_DISABLE_WARNING(4456)
#define KX_DISABLE_WARNING_MISSING_FIELD_INITIALIZERS KX_DISABLE_WARNING(4127)
#define KX_DISABLE_WARNING_NAMELESS_STRUCT KX_DISABLE_WARNING(4201)
#define KX_DISABLE_WARNING_SIGNED_UNSIGNED_COMPARISON_MISMATCH KX_DISABLE_WARNING(4018)
#define KX_DISABLE_WARNING_SIGNED_UNSIGNED_ASSIGNMENT_MISMATCH KX_DISABLE_WARNING(4245)
#define KX_DISABLE_WARNING_SIZE_T_CONVERSION KX_DISABLE_WARNING(4267)
#define KX_DISABLE_WARNING_CONVERSION
#define KX_DISABLE_WARNING_UNREFERENCED_LOCAL_VARIABLE KX_DISABLE_WARNING(4101)
#define KX_DISABLE_WARNING_UNUSED_PARAMETER KX_DISABLE_WARNING(4100)
#define KX_DISABLE_WARNING_UNUSED_FUNCTION KX_DISABLE_WARNING(4505)
#define KX_DISABLE_WARNING_UNUSED_VARIABLE KX_DISABLE_WARNING(4101)
#define KX_DISABLE_WARNING_OUTSIDE_RANGE KX_DISABLE_WARNING(4244)
#endif
// NOLINTEND
// clang-format on

#undef None

#if defined(DEBUG)
constexpr bool USE_VALIDATION_LAYERS = true;
#else
constexpr bool USE_VALIDATION_LAYERS = false;
#endif

constexpr uint8_t MAX_FRAMES_IN_FLIGHT = 4;
constexpr uint8_t MAX_DEPTH_PYRAMID_LEVELS = 10;

constexpr int VERTEX_ATTRIBUTE_COUNT = sizeof(Vertex) / sizeof(float);

constexpr float VERTEX_ATTRIBUTE_WEIGHT_NORMAL = 0.5f;
constexpr float VERTEX_ATTRIBUTE_WEIGHT_UV = 1.0f;
constexpr float VERTEX_ATTRIBUTE_WEIGHT_COLOR = 0.0f;
constexpr float VERTEX_ATTRIBUTE_WEIGHT_TANGENT = 0.3f;

constexpr float VERTEX_ATTRIBUTE_WEIGHTS[] = {
    VERTEX_ATTRIBUTE_WEIGHT_NORMAL,  // normal xyz
    VERTEX_ATTRIBUTE_WEIGHT_NORMAL,
    VERTEX_ATTRIBUTE_WEIGHT_NORMAL,
    VERTEX_ATTRIBUTE_WEIGHT_UV,     // uv_x
    VERTEX_ATTRIBUTE_WEIGHT_COLOR,  // color rgba
    VERTEX_ATTRIBUTE_WEIGHT_COLOR,
    VERTEX_ATTRIBUTE_WEIGHT_COLOR,
    VERTEX_ATTRIBUTE_WEIGHT_COLOR,
    VERTEX_ATTRIBUTE_WEIGHT_TANGENT,  // tangent xyzw
    VERTEX_ATTRIBUTE_WEIGHT_TANGENT,
    VERTEX_ATTRIBUTE_WEIGHT_TANGENT,
    VERTEX_ATTRIBUTE_WEIGHT_TANGENT,
    VERTEX_ATTRIBUTE_WEIGHT_UV,  // uv_y
};

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) { deletors.push_back(std::move(function)); }
    void flush()
    {
        for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) (*it)();
        deletors.clear();
    }
};

struct AllocatedImage
{
    VkImage image;
    VkImageView view;
    VmaAllocation allocation;
    VkExtent3D extent;
    VkFormat format;
};

struct AllocatedBuffer
{
    VkBuffer buffer{VK_NULL_HANDLE};
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

namespace kynetic
{

enum class RenderMode
{
    CpuDriven,
    GpuDriven,
    Meshlets
};

struct Resource
{
    enum class Type
    {
        Shader,
        Material,
        Texture,
        Model,
        Mesh,
    };

    Type type;
    std::string path;

    size_t id{0};
    bool is_loaded{false};

    Resource(const Type type, std::string path) : type(type), path(std::move(path)) {}
    virtual ~Resource() = default;
};

}  // namespace kynetic

namespace vk_init
{
VkCommandPoolCreateInfo command_pool_create_info(uint32_t queue_family_index, VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1);

VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer command_buffer);

VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* command_buffer_info,
                          VkSemaphoreSubmitInfo* signal_semaphore_info,
                          VkSemaphoreSubmitInfo* wait_semaphore_info);
VkPresentInfoKHR present_info();

VkRenderingAttachmentInfo attachment_info(VkImageView view,
                                          VkClearValue* clear,
                                          VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingAttachmentInfo depth_attachment_info(VkImageView view,
                                                VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingInfo rendering_info(VkExtent2D render_extent,
                               VkRenderingAttachmentInfo* color_attachment,
                               VkRenderingAttachmentInfo* depth_attachment);

VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspect_mask);

VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);
VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type,
                                                          VkShaderStageFlags stage_flags,
                                                          uint32_t binding);
VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info(VkDescriptorSetLayoutBinding* bindings,
                                                                 uint32_t binding_count);
VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type,
                                            VkDescriptorSet set,
                                            VkDescriptorImageInfo* imageInfo,
                                            uint32_t binding);
VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type,
                                             VkDescriptorSet set,
                                             VkDescriptorBufferInfo* bufferInfo,
                                             uint32_t binding);
VkDescriptorBufferInfo buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent);
VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags);
VkPipelineLayoutCreateInfo pipeline_layout_create_info();
VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                                                                  VkShaderModule shader_module,
                                                                  const char* entry = "main");
}  // namespace vk_init

namespace vk_util
{

void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

VkDescriptorType slang_to_vk_descriptor_type(slang::BindingType type);

VkShaderStageFlags slang_to_vk_stage(SlangStage stage);
VkShaderStageFlagBits slang_to_vk_stage_bit(const SlangStage stage);
}  // namespace vk_util

glm::mat4 make_transform_matrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

template <typename EnumType>
static void combo_enum(EnumType& enum_)
{
    const auto type_name = std::string(magic_enum::enum_type_name<EnumType>());
    auto current_name = magic_enum::enum_name(enum_);
    const std::string preview = current_name.empty() ? "Unknown" : std::string(current_name);

    // ImGui::PushItemWidth(150.0f);
    if (ImGui::BeginCombo(type_name.c_str(), preview.c_str()))
    {
        constexpr auto enum_values = magic_enum::enum_values<EnumType>();
        for (const auto& enum_value : enum_values)
        {
            auto enum_name = magic_enum::enum_name(enum_value);
            const bool is_selected = (enum_ == enum_value);

            if (ImGui::Selectable(std::string(enum_name).c_str(), is_selected))
            {
                enum_ = enum_value;
            }

            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}