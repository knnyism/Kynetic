#pragma once

#include <print>
#include <fstream>
#include <filesystem>
#include <cmath>

#include "imgui.h"
#include "ImGuizmo.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "vulkan/vulkan_core.h"

#include "VkBootstrap.h"

std::vector<char> ReadFile(const std::string& filename);