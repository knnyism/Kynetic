//
// Created by kenny on 7/1/25.
//

#include "Engine/Core/Window.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Swapchain.h"

#include "App.h"

namespace Kynetic {

    constexpr int MAX_FRAMES_IN_FLIGHT = 3;


    int App::CreateRenderPassVulkan(RenderData& data) const {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = m_swapchain->get_image_format();
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

        if (m_device->m_disp.createRenderPass(&renderPassInfo, nullptr, &data.renderPass) != VK_SUCCESS) {
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
        if (m_device->m_disp.createShaderModule(&createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
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
        viewport.width = static_cast<float>(m_swapchain->get_extent().width);
        viewport.height = static_cast<float>(m_swapchain->get_extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapchain->get_extent();

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

        if (m_device->m_disp.createPipelineLayout(&pipelineLayoutInfo, nullptr, &data.pipelineLayout) != VK_SUCCESS) {
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

        if (m_device->m_disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &data.graphicsPipeline) != VK_SUCCESS) {
            std::println("Failed to create graphics pipeline");
            return -1;
        }

        m_device->m_disp.destroyShaderModule(fragModule, nullptr);
        m_device->m_disp.destroyShaderModule(vertModule, nullptr);

        return 0;
    }

    int App::CreateFramebuffersVulkan(RenderData& data) const {
        const auto& swapchain_image_views = m_swapchain->get_image_views();
        data.framebuffers.resize(swapchain_image_views.size());

        for (size_t i = 0; i < swapchain_image_views.size(); i++) {
            VkImageView attachments[] = { swapchain_image_views[i] };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = data.renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapchain->get_extent().width;
            framebufferInfo.height = m_swapchain->get_extent().height;
            framebufferInfo.layers = 1;

            if (m_device->m_disp.createFramebuffer(&framebufferInfo, nullptr, &data.framebuffers[i]) != VK_SUCCESS) {
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
        poolInfo.queueFamilyIndex = m_device->get_device().get_queue_index(vkb::QueueType::graphics).value();

        if (m_device->m_disp.createCommandPool(&poolInfo, nullptr, &data.commandPool) != VK_SUCCESS) {
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

        if (m_device->m_disp.allocateCommandBuffers(&allocInfo, data.commandBuffers.data()) != VK_SUCCESS) {
            return -1;
        }

        return 0;
    }

    void App::recreate_swapchain(RenderData& data) {
        m_device->m_disp.deviceWaitIdle();

        m_device->m_disp.destroyCommandPool(data.commandPool, nullptr);

        for (const auto framebuffer : data.framebuffers) {
            m_device->m_disp.destroyFramebuffer(framebuffer, nullptr);
        }

        m_swapchain->create_swapchain();

        CreateFramebuffersVulkan(data);
        CreateCommandPoolVulkan(data);
        CreateCommandBuffersVulkan(data);
    }

    int App::DrawFrame(RenderData& data) {
        if (m_window_resized) {
            m_window_resized = false;
            recreate_swapchain(data);
            return 0;
        }

        const uint32_t image_index = m_swapchain->acquire_next_image_index();

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        m_device->m_disp.resetCommandBuffer(data.commandBuffers[image_index], 0);
        m_device->m_disp.beginCommandBuffer(data.commandBuffers[image_index], &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = data.renderPass;
        renderPassInfo.framebuffer = data.framebuffers[image_index];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapchain->get_extent();

        constexpr VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        m_device->m_disp.cmdBeginRenderPass(data.commandBuffers[image_index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapchain->get_extent().width);
        viewport.height = static_cast<float>(m_swapchain->get_extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapchain->get_extent();

        m_device->m_disp.cmdSetViewport(data.commandBuffers[image_index], 0, 1, &viewport);
        m_device->m_disp.cmdSetScissor(data.commandBuffers[image_index], 0, 1, &scissor);
        m_device->m_disp.cmdBindPipeline(data.commandBuffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, data.graphicsPipeline);
        m_device->m_disp.cmdDraw(data.commandBuffers[image_index], 3, 1, 0, 0);

        // Render ImGui
        if (ImDrawData* draw_data = ImGui::GetDrawData()) {
            ImGui_ImplVulkan_RenderDrawData(draw_data, data.commandBuffers[image_index]);
        }

        m_device->m_disp.cmdEndRenderPass(data.commandBuffers[image_index]);
        m_device->m_disp.endCommandBuffer(data.commandBuffers[image_index]);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        const VkSemaphore waitSemaphores[] = { m_swapchain->get_available_semaphore() };
        constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &data.commandBuffers[image_index];

        const VkSemaphore signalSemaphores[] = { m_swapchain->get_finished_semaphore(image_index) };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        m_swapchain->reset_fence();

        if (m_device->m_disp.queueSubmit(m_device->get_graphics_queue(), 1, &submitInfo, m_swapchain->get_in_flight_fence()) != VK_SUCCESS) {
            std::println("Failed to submit draw command buffer");
            return -1;
        }

        m_swapchain->present(image_index);

        return 0;
    }

    int App::InitializeVulkan(RenderData& data) {
        if (0 != CreateRenderPassVulkan(data)) return -1;
        if (0 != CreateGraphicsPipelineVulkan(data)) return -1;
        if (0 != CreateFramebuffersVulkan(data)) return -1;
        if (0 != CreateCommandPoolVulkan(data)) return -1;
        if (0 != CreateCommandBuffersVulkan(data)) return -1;

        return 0;
    }

    void App::Cleanup(const RenderData& data) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_device->m_disp.destroyCommandPool(data.commandPool, nullptr);

        for (const auto framebuffer : data.framebuffers) {
            m_device->m_disp.destroyFramebuffer(framebuffer, nullptr);
        }

        m_device->m_disp.destroyPipeline(data.graphicsPipeline, nullptr);
        m_device->m_disp.destroyPipelineLayout(data.pipelineLayout, nullptr);
        m_device->m_disp.destroyRenderPass(data.renderPass, nullptr);
    }

    App::App() : m_window(std::make_unique<Window>(1024, 768, "Kynetic")),
                 m_device(std::make_unique<Device>(m_window->get())),
                 m_swapchain(std::make_unique<Swapchain>(*m_device)) {
    }

    App::~App() = default;

    void App::Start() {
        RenderData data;

        InitializeCallbacks();
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
        m_device->m_disp.deviceWaitIdle();

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

        ImGui_ImplGlfw_InitForVulkan(m_window->get(), true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = m_device->get_instance().api_version;
        initInfo.Instance = m_device->get_instance().instance;
        initInfo.PhysicalDevice = m_device->get_device().physical_device;
        initInfo.Device = m_device->get_device().device;
        initInfo.QueueFamily = m_device->get_device().get_queue_index(vkb::QueueType::graphics).value();
        initInfo.Queue = m_device->get_graphics_queue();
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPoolSize = m_swapchain->get_image_count() * 1024;
        initInfo.RenderPass = data.renderPass;
        initInfo.MinImageCount = m_swapchain->get_image_count();
        initInfo.ImageCount = m_swapchain->get_image_count();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Subpass = 0;

        ImGui_ImplVulkan_Init(&initInfo);
    }

    void App::InitializeCallbacks() {
        m_window->set_user_pointer(this);

        m_window->set_resize_callback([](GLFWwindow *window, int width, int height) {
            auto *app = static_cast<App*>(glfwGetWindowUserPointer(window));
            std::println("Window extents: {}x{}", width, height);
            app->m_window_resized = true;
        });
    }
} // Kynetic
