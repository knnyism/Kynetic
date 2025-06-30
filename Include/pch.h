#pragma once

#include <print>

#include "imgui.h"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan_core.h"

#include "VkBootstrap.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

struct RenderData {
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> availableSemaphores;
    std::vector<VkSemaphore> finishedSemaphore;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imageInFlight;
    size_t currentFrame = 0;
};
