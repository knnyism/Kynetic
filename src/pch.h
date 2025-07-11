#pragma once

#define GLFW_INCLUDE_VULKAN

#include <print>
#include <fstream>
#include <filesystem>
#include <utility>
#include <cmath>

#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "GLFW/glfw3.h"

#include "glm/glm.hpp"

#include "vulkan/vulkan_core.h"
#include "VkBootstrap.h"

// #define VMA_DEBUG_LOG(format, ...) do { \
// printf(format __VA_OPT__(,) __VA_ARGS__); \
// printf("\n"); \
// } while(false)

#ifdef _MSC_VER
#pragma warning(push, 4)
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4189) // local variable is initialized but not referenced
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
#pragma warning(disable: 4820) // 'X': 'N' bytes padding added after data member 'X'
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include "vk_mem_alloc.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

std::vector<char> read_file(const std::string& filename);