//
// Created by kenny on 7/1/25.
//

#include "Window.h"
#include "App.h"

namespace Kynetic {

    constexpr int MAX_FRAMES_IN_FLIGHT = 3;

    int App::InitializeDeviceVulkan() {
        vkb::InstanceBuilder instanceBuilder;
        auto instanceResult = instanceBuilder.use_default_debug_messenger().request_validation_layers().build();
        if (!instanceResult) {
            std::println("{}", instanceResult.error().message());
            return -1;
        }
        instance = instanceResult.value();

        instanceDispatch = instance.make_table();

        surface = m_window->create_surface(instance, nullptr);

        vkb::PhysicalDeviceSelector physicalDeviceSelector(instance);
        auto physicalDeviceResult = physicalDeviceSelector.set_surface(surface).select();
        if (!physicalDeviceResult) {
            std::println("{}", physicalDeviceResult.error().message());
            return -1;
        }

        vkb::DeviceBuilder deviceBuilder(physicalDeviceResult.value());
        auto deviceResult = deviceBuilder.build();
        if (!deviceResult) {
            std::println("{}", deviceResult.error().message());
            return -1;
        }
        device = deviceResult.value();

        dispatch = device.make_table();

        return 0;
    }

    int App::CreateSwapchainVulkan() {
        vkb::SwapchainBuilder swapchainBuilder(device);
        auto swapchainResult = swapchainBuilder
            .set_old_swapchain(swapchain)
            .build();
        if (!swapchainResult) {
            std::println("{} {}", swapchainResult.error().message(), static_cast<int>(swapchainResult.vk_result()));
            return -1;
        }
        vkb::destroy_swapchain(swapchain);
        swapchain = swapchainResult.value();

        return 0;
    }

    int App::GetQueuesVulkan(RenderData& data) const {
        auto graphicsQueueResult = device.get_queue(vkb::QueueType::graphics);
        if (!graphicsQueueResult.has_value()) {
            std::println("Failed to get graphics queue: {}", graphicsQueueResult.error().message());
            return -1;
        }
        data.graphicsQueue = graphicsQueueResult.value();

        auto presentQueueResult = device.get_queue(vkb::QueueType::present);
        if (!presentQueueResult.has_value()) {
            std::println("Failed to get present queue: {}", presentQueueResult.error().message());
            return -1;
        }
        data.presentQueue = presentQueueResult.value();

        return 0;
    }

    int App::CreateRenderPassVulkan(RenderData& data) const {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = swapchain.image_format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (dispatch.createRenderPass(&renderPassInfo, nullptr, &data.renderPass) != VK_SUCCESS) {
            std::println("Failed to create render pass");
            return -1;
        }

        return 0;
    }

    VkShaderModule App::CreateShaderModuleVulkan(const std::vector<char>& code) const {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (dispatch.createShaderModule(&createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }

    int App::CreateGraphicsPipelineVulkan(RenderData& data) const {
        auto vertCode = ReadFile(std::string(KYNETIC_SOURCE_DIR) + "/assets/shaders/triangle.vert.spv");
        auto fragCode = ReadFile(std::string(KYNETIC_SOURCE_DIR) + "/assets/shaders/triangle.frag.spv");

        VkShaderModule vertModule = CreateShaderModuleVulkan(vertCode);
        VkShaderModule fragModule = CreateShaderModuleVulkan(fragCode);
        if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
            std::println("Failed to create shader modules");
            return -1;
        }

        VkPipelineShaderStageCreateInfo vertStageInfo = {};
        vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module = vertModule;
        vertStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragStageInfo = {};
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = fragModule;
        fragStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchain.extent.width);
        viewport.height = static_cast<float>(swapchain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = swapchain.extent;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (dispatch.createPipelineLayout(&pipelineLayoutInfo, nullptr, &data.pipelineLayout) != VK_SUCCESS) {
            std::println("Failed to create pipeline layout");
            return -1;
        }

        std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamicInfo = {};
        dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicInfo.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicInfo;
        pipelineInfo.layout = data.pipelineLayout;
        pipelineInfo.renderPass = data.renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (dispatch.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &data.graphicsPipeline) != VK_SUCCESS) {
            std::println("Failed to create graphics pipeline");
            return -1;
        }

        dispatch.destroyShaderModule(fragModule, nullptr);
        dispatch.destroyShaderModule(vertModule, nullptr);

        return 0;
    }

    int App::CreateFramebuffersVulkan(RenderData& data) {
        data.swapchainImages = swapchain.get_images().value();
        data.swapchainImageViews = swapchain.get_image_views().value();

        data.framebuffers.resize(data.swapchainImageViews.size());

        for (size_t i = 0; i < data.swapchainImageViews.size(); i++) {
            VkImageView attachments[] = { data.swapchainImageViews[i] };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = data.renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapchain.extent.width;
            framebufferInfo.height = swapchain.extent.height;
            framebufferInfo.layers = 1;

            if (dispatch.createFramebuffer(&framebufferInfo, nullptr, &data.framebuffers[i]) != VK_SUCCESS) {
                std::println("Failed to create framebuffer");
                return -1;
            }
        }

        return 0;
    }

    int App::CreateCommandPoolVulkan(RenderData& data) const {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();

        if (dispatch.createCommandPool(&poolInfo, nullptr, &data.commandPool) != VK_SUCCESS) {
            std::println("Failed to create command pool");
            return -1;
        }

        return 0;
    }

    int App::CreateCommandBuffersVulkan(RenderData& data) const {
        data.commandBuffers.resize(data.framebuffers.size());

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = data.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(data.commandBuffers.size());

        if (dispatch.allocateCommandBuffers(&allocInfo, data.commandBuffers.data()) != VK_SUCCESS) {
            return -1;
        }

        return 0;
    }

    int App::CreateSyncObjectsVulkan(RenderData& data) const {
        data.availableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        data.finishedSemaphore.resize(swapchain.image_count);
        data.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        data.imageInFlight.resize(swapchain.image_count, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < swapchain.image_count; i++) {
            if (dispatch.createSemaphore(&semaphoreInfo, nullptr, &data.finishedSemaphore[i]) != VK_SUCCESS) {
                return -1; // failed to create synchronization objects for a frame
            }
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (dispatch.createSemaphore(&semaphoreInfo, nullptr, &data.availableSemaphores[i]) != VK_SUCCESS ||
                dispatch.createFence(&fenceInfo, nullptr, &data.inFlightFences[i]) != VK_SUCCESS) {
                return -1; // failed to create synchronization objects for a frame
                }
        }
        return 0;
    }

    int App::RecreateSwapchainVulkan(RenderData& data) {
        dispatch.deviceWaitIdle();

        dispatch.destroyCommandPool(data.commandPool, nullptr);

        for (auto framebuffer : data.framebuffers) {
            dispatch.destroyFramebuffer(framebuffer, nullptr);
        }

        swapchain.destroy_image_views(data.swapchainImageViews);

        if (0 != CreateSwapchainVulkan()) return -1;
        if (0 != CreateFramebuffersVulkan(data)) return -1;
        if (0 != CreateCommandPoolVulkan(data)) return -1;
        if (0 != CreateCommandBuffersVulkan(data)) return -1;

        return 0;
    }

    int App::DrawFrame(RenderData& data) {
        dispatch.waitForFences(1, &data.inFlightFences[data.currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex = 0;
        VkResult result = dispatch.acquireNextImageKHR(
            swapchain, UINT64_MAX, data.availableSemaphores[data.currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return RecreateSwapchainVulkan(data);
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return -1;
        }

        if (data.imageInFlight[imageIndex] != VK_NULL_HANDLE) {
            dispatch.waitForFences(1, &data.imageInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        data.imageInFlight[imageIndex] = data.inFlightFences[data.currentFrame];

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        dispatch.resetCommandBuffer(data.commandBuffers[imageIndex], 0);
        dispatch.beginCommandBuffer(data.commandBuffers[imageIndex], &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = data.renderPass;
        renderPassInfo.framebuffer = data.framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapchain.extent;

        constexpr VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        dispatch.cmdBeginRenderPass(data.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchain.extent.width);
        viewport.height = static_cast<float>(swapchain.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = swapchain.extent;

        dispatch.cmdSetViewport(data.commandBuffers[imageIndex], 0, 1, &viewport);
        dispatch.cmdSetScissor(data.commandBuffers[imageIndex], 0, 1, &scissor);
        dispatch.cmdBindPipeline(data.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, data.graphicsPipeline);
        dispatch.cmdDraw(data.commandBuffers[imageIndex], 3, 1, 0, 0);

        // Render ImGui
        if (ImDrawData* draw_data = ImGui::GetDrawData()) {
            ImGui_ImplVulkan_RenderDrawData(draw_data, data.commandBuffers[imageIndex]);
        }

        dispatch.cmdEndRenderPass(data.commandBuffers[imageIndex]);
        dispatch.endCommandBuffer(data.commandBuffers[imageIndex]);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        const VkSemaphore waitSemaphores[] = { data.availableSemaphores[data.currentFrame] };
        constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &data.commandBuffers[imageIndex];

        const VkSemaphore signalSemaphores[] = { data.finishedSemaphore[imageIndex] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        dispatch.resetFences(1, &data.inFlightFences[data.currentFrame]);

        if (dispatch.queueSubmit(data.graphicsQueue, 1, &submitInfo, data.inFlightFences[data.currentFrame]) != VK_SUCCESS) {
            std::println("Failed to submit draw command buffer");
            return -1;
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        const VkSwapchainKHR swapchains[] = { swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        result = dispatch.queuePresentKHR(data.presentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return RecreateSwapchainVulkan(data);
        } else if (result != VK_SUCCESS) {
            return -1;
        }

        data.currentFrame = (data.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        return 0;
    }

    int App::InitializeVulkan(RenderData& data) {
        if (0 != InitializeDeviceVulkan()) return -1;
        if (0 != CreateSwapchainVulkan()) return -1;
        if (0 != GetQueuesVulkan(data)) return -1;
        if (0 != CreateRenderPassVulkan(data)) return -1;
        if (0 != CreateGraphicsPipelineVulkan(data)) return -1;
        if (0 != CreateFramebuffersVulkan(data)) return -1;
        if (0 != CreateCommandPoolVulkan(data)) return -1;
        if (0 != CreateCommandBuffersVulkan(data)) return -1;
        if (0 != CreateSyncObjectsVulkan(data)) return -1;

        return 0;
    }

    void App::Cleanup(const RenderData& data) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        for (size_t i = 0; i < swapchain.image_count; i++) {
            dispatch.destroySemaphore(data.finishedSemaphore[i], nullptr);
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            dispatch.destroySemaphore(data.availableSemaphores[i], nullptr);
            dispatch.destroyFence(data.inFlightFences[i], nullptr);
        }

        dispatch.destroyCommandPool(data.commandPool, nullptr);

        for (auto framebuffer : data.framebuffers) {
            dispatch.destroyFramebuffer(framebuffer, nullptr);
        }

        dispatch.destroyPipeline(data.graphicsPipeline, nullptr);
        dispatch.destroyPipelineLayout(data.pipelineLayout, nullptr);
        dispatch.destroyRenderPass(data.renderPass, nullptr);

        swapchain.destroy_image_views(data.swapchainImageViews);

        vkb::destroy_swapchain(swapchain);
        vkb::destroy_device(device);
        vkb::destroy_surface(instance, surface);
        vkb::destroy_instance(instance);
    }

    App::App() {
        m_window =
            std::make_unique<Window>(1024, 768, "Kynetic");
    }

    void App::Start() {
        RenderData data;

        InitializeVulkan(data);
        InitializeImGui(data);

        while (!m_window->should_close()) {
            glfwPollEvents();

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();

            ImGui::ShowDemoWindow();
            ImGui::Render();

            if (const int res = DrawFrame(data); res != 0) {
                return;
            }
        }
        dispatch.deviceWaitIdle();

        Cleanup(data);
    }

    void App::InitializeImGui(const RenderData& data) const {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.3f;
        style.FrameRounding = 2.3f;
        style.ScrollbarRounding = 0.0f;
        style.Colors[ImGuiCol_Text]                      = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled]              = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_WindowBg]                  = ImVec4(0.07f, 0.02f, 0.02f, 0.85f);
        style.Colors[ImGuiCol_ChildBg]                   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_PopupBg]                   = ImVec4(0.14f, 0.11f, 0.11f, 0.92f);
        style.Colors[ImGuiCol_Border]                    = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
        style.Colors[ImGuiCol_BorderShadow]              = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg]                   = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
        style.Colors[ImGuiCol_FrameBgHovered]            = ImVec4(0.70f, 0.41f, 0.41f, 0.40f);
        style.Colors[ImGuiCol_FrameBgActive]             = ImVec4(0.75f, 0.48f, 0.48f, 0.69f);
        style.Colors[ImGuiCol_TitleBg]                   = ImVec4(0.48f, 0.18f, 0.18f, 0.65f);
        style.Colors[ImGuiCol_TitleBgActive]             = ImVec4(0.52f, 0.12f, 0.12f, 0.87f);
        style.Colors[ImGuiCol_TitleBgCollapsed]          = ImVec4(0.80f, 0.40f, 0.40f, 0.20f);
        style.Colors[ImGuiCol_MenuBarBg]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.80f);
        style.Colors[ImGuiCol_ScrollbarBg]               = ImVec4(0.30f, 0.20f, 0.20f, 0.60f);
        style.Colors[ImGuiCol_ScrollbarGrab]             = ImVec4(0.96f, 0.17f, 0.17f, 0.30f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered]      = ImVec4(1.00f, 0.07f, 0.07f, 0.40f);
        style.Colors[ImGuiCol_ScrollbarGrabActive]       = ImVec4(1.00f, 0.36f, 0.36f, 0.60f);
        style.Colors[ImGuiCol_CheckMark]                 = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
        style.Colors[ImGuiCol_SliderGrab]                = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
        style.Colors[ImGuiCol_SliderGrabActive]          = ImVec4(0.80f, 0.39f, 0.39f, 0.60f);
        style.Colors[ImGuiCol_Button]                    = ImVec4(0.71f, 0.18f, 0.18f, 0.62f);
        style.Colors[ImGuiCol_ButtonHovered]             = ImVec4(0.71f, 0.27f, 0.27f, 0.79f);
        style.Colors[ImGuiCol_ButtonActive]              = ImVec4(0.80f, 0.46f, 0.46f, 1.00f);
        style.Colors[ImGuiCol_Header]                    = ImVec4(0.56f, 0.16f, 0.16f, 0.45f);
        style.Colors[ImGuiCol_HeaderHovered]             = ImVec4(0.53f, 0.11f, 0.11f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive]              = ImVec4(0.87f, 0.53f, 0.53f, 0.80f);
        style.Colors[ImGuiCol_Separator]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
        style.Colors[ImGuiCol_SeparatorHovered]          = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
        style.Colors[ImGuiCol_SeparatorActive]           = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip]                = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
        style.Colors[ImGuiCol_ResizeGripHovered]         = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
        style.Colors[ImGuiCol_ResizeGripActive]          = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
        style.Colors[ImGuiCol_TabHovered]                = ImVec4(0.68f, 0.21f, 0.21f, 0.80f);
        style.Colors[ImGuiCol_Tab]                       = ImVec4(0.47f, 0.12f, 0.12f, 0.79f);
        style.Colors[ImGuiCol_TabSelected]               = ImVec4(0.68f, 0.21f, 0.21f, 1.00f);
        style.Colors[ImGuiCol_TabSelectedOverline]       = ImVec4(0.95f, 0.84f, 0.84f, 0.40f);
        style.Colors[ImGuiCol_TabDimmed]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
        style.Colors[ImGuiCol_TabDimmedSelected]         = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
        style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.55f, 0.23f, 0.23f, 1.00f);
        style.Colors[ImGuiCol_DockingPreview]            = ImVec4(0.90f, 0.40f, 0.40f, 0.31f);
        style.Colors[ImGuiCol_DockingEmptyBg]            = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_PlotLines]                 = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram]             = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered]      = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TableHeaderBg]             = ImVec4(0.56f, 0.16f, 0.16f, 0.45f);
        style.Colors[ImGuiCol_TableBorderStrong]         = ImVec4(0.68f, 0.21f, 0.21f, 0.80f);
        style.Colors[ImGuiCol_TableBorderLight]          = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
        style.Colors[ImGuiCol_TableRowBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_TableRowBgAlt]             = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        style.Colors[ImGuiCol_TextSelectedBg]            = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
        style.Colors[ImGuiCol_DragDropTarget]            = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        style.Colors[ImGuiCol_NavHighlight]              = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
        style.Colors[ImGuiCol_NavWindowingHighlight]     = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        style.Colors[ImGuiCol_NavWindowingDimBg]         = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        style.Colors[ImGuiCol_ModalWindowDimBg]          = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

        ImGui_ImplGlfw_InitForVulkan(m_window->get_handle(), true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = instance.api_version;
        initInfo.Instance = instance.instance;
        initInfo.PhysicalDevice = device.physical_device;
        initInfo.Device = device.device;
        initInfo.QueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();
        initInfo.Queue = data.graphicsQueue;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPoolSize = swapchain.image_count * 1024;
        initInfo.RenderPass = data.renderPass;
        initInfo.MinImageCount = swapchain.image_count;
        initInfo.ImageCount = swapchain.image_count;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Subpass = 0;

        ImGui_ImplVulkan_Init(&initInfo);
    }
} // Kynetic
