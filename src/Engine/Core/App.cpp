//
// Created by kenny on 7/1/25.
//

#include "Engine/Core/Window.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Swapchain.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Renderer.h"

#include "App.h"

namespace Kynetic {
    int App::CreateCommandPoolVulkan() const {
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

    int App::CreateCommandBuffersVulkan() const {
        data.commandBuffers.resize(m_swapchain->get_framebuffers().size());

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

    void App::recreate_swapchain() const {
        m_device->m_disp.deviceWaitIdle();

        m_device->m_disp.destroyCommandPool(data.commandPool, nullptr);

        m_swapchain->create_swapchain();
        m_swapchain->create_framebuffers(m_renderer->get_render_pass());

        CreateCommandPoolVulkan();
        CreateCommandBuffersVulkan();
    }

    int App::DrawFrame() {
        if (m_window_resized) {
            m_window_resized = false;
            recreate_swapchain();
            return 0;
        }

        const uint32_t image_index = m_swapchain->acquire_next_image_index();

        if (image_index == UINT32_MAX) {
            recreate_swapchain();
            return 0;
        }

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        m_device->m_disp.resetCommandBuffer(data.commandBuffers[image_index], 0);
        m_device->m_disp.beginCommandBuffer(data.commandBuffers[image_index], &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderer->get_render_pass();
        renderPassInfo.framebuffer = m_swapchain->get_framebuffers()[image_index];
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
        m_device->m_disp.cmdBindPipeline(data.commandBuffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderer->get_pipeline());
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

    int App::InitializeVulkan() {
        if (0 != CreateCommandPoolVulkan()) return -1;
        if (0 != CreateCommandBuffersVulkan()) return -1;

        return 0;
    }

    void App::Cleanup() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_device->m_disp.destroyCommandPool(data.commandPool, nullptr);
    }

    App::App() : m_window(std::make_unique<Window>(1024, 768, "Kynetic")),
                 m_device(std::make_unique<Device>(m_window->get())),
                 m_swapchain(std::make_unique<Swapchain>(*m_device)),
                 m_renderer(std::make_unique<Renderer>(*m_device, *m_swapchain)) {
        m_swapchain->create_framebuffers(m_renderer->get_render_pass());
    }

    App::~App() = default;

    void App::Start() {
        InitializeCallbacks();
        InitializeVulkan();
        InitializeImGui();

        while (!m_window->should_close()) {
            glfwPollEvents();

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();

            ImGui::ShowDemoWindow();
            ImGui::Render();

            if (const int res = DrawFrame(); res != 0) {
                return;
            }
        }
        m_device->m_disp.deviceWaitIdle();

        Cleanup();
    }

    void App::InitializeImGui() const {
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
        initInfo.RenderPass = m_renderer->get_render_pass();
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
            app->m_window_resized = true;
        });
    }
} // Kynetic
