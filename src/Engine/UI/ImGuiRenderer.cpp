//
// Created by kenny on 5-7-25.
//

#include "Engine/Core/Window.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Swapchain.h"

#include "ImGuiRenderer.h"

namespace Kynetic {

ImGuiRenderer::ImGuiRenderer(const Window &window, const Device &device, const Swapchain &swapchain,
                             const VkRenderPass render_pass) {
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

    ImGui_ImplGlfw_InitForVulkan(window.get(), true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = device.get_instance().api_version;
    initInfo.Instance = device.get_instance().instance;
    initInfo.PhysicalDevice = device.get_device().physical_device;
    initInfo.Device = device.get_device().device;
    initInfo.QueueFamily = device.get_device().get_queue_index(vkb::QueueType::graphics).value();
    initInfo.Queue = device.get_graphics_queue();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPoolSize = swapchain.get_image_count() * 1024;
    initInfo.RenderPass = render_pass;
    initInfo.MinImageCount = swapchain.get_image_count();
    initInfo.ImageCount = swapchain.get_image_count();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Subpass = 0;

    ImGui_ImplVulkan_Init(&initInfo);
}

ImGuiRenderer::~ImGuiRenderer() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiRenderer::new_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void ImGuiRenderer::render() {
    ImGui::Render();
}

void ImGuiRenderer::render_draw_data(const VkCommandBuffer& cmd_buf) {
    if (ImDrawData* draw_data = ImGui::GetDrawData()) {
        ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf);
    }
}

} // Kynetic