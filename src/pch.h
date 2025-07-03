#pragma once

#include <print>
#include <fstream>
#include <filesystem>
#include <cmath>

#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"
#include "VkBootstrap.h"

#include "imgui.h"
#include "ImGuizmo.h"

std::vector<char> ReadFile(const std::string& filename);