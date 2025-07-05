#pragma once

#define GLFW_INCLUDE_VULKAN

#include <print>
#include <fstream>
#include <filesystem>
#include <utility>
#include <cmath>

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"
#include "VkBootstrap.h"

#include "imgui.h"
#include "ImGuizmo.h"

std::vector<char> ReadFile(const std::string& filename);