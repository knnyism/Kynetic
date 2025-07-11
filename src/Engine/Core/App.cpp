//
// Created by kenny on 7/1/25.
//

#include "Engine/Core/Window.h"
#include "Engine/Renderer/Device.h"
#include "Engine/Renderer/Swapchain.h"
#include "Engine/Renderer/CommandPool.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/UI/ImGuiRenderer.h"

#include "App.h"

namespace Kynetic {

App::App() {
    glfwInit();

    m_window = std::make_unique<Window>(1024, 768, "Kynetic");

    m_device = std::make_unique<Device>(m_window->get());
    m_swapchain = std::make_unique<Swapchain>(*m_device);
    m_cmd_pool = std::make_unique<CommandPool>(*m_device);

    create_render_pass();

    m_swapchain->create_framebuffers(m_render_pass);
    m_cmd_bufs = m_cmd_pool->allocate_buffers(m_swapchain->get_image_count());

    create_graphics_pipeline();
    initialize_callbacks();

    m_imgui_renderer = std::make_unique<ImGuiRenderer>(*m_window, *m_device, *m_swapchain, m_render_pass);
}

App::~App() = default;

void App::recreate_swapchain() const {
    m_device->m_disp.deviceWaitIdle();

    m_swapchain->create_swapchain();
    m_swapchain->create_framebuffers(m_render_pass);
}

int App::draw_frame() {
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

    buffers_queued_for_destruction.clear();

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    m_device->m_disp.resetCommandBuffer(m_cmd_bufs[image_index], 0);
    m_device->m_disp.beginCommandBuffer(m_cmd_bufs[image_index], &beginInfo);

    create_vertex_buffer(m_cmd_bufs[image_index], buffers_queued_for_destruction);
    create_index_buffer(m_cmd_bufs[image_index], buffers_queued_for_destruction);

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_render_pass;
    renderPassInfo.framebuffer = m_swapchain->get_framebuffers()[image_index];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapchain->get_extent();

    constexpr VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    m_device->m_disp.cmdBeginRenderPass(m_cmd_bufs[image_index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

    m_device->m_disp.cmdSetViewport(m_cmd_bufs[image_index], 0, 1, &viewport);
    m_device->m_disp.cmdSetScissor(m_cmd_bufs[image_index], 0, 1, &scissor);
    m_device->m_disp.cmdBindPipeline(m_cmd_bufs[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    const VkBuffer vertexBuffers[] = {m_vertex_buffer->get()};
    constexpr VkDeviceSize offsets[] = {0};

    m_device->m_disp.cmdBindVertexBuffers(m_cmd_bufs[image_index], 0, 1, vertexBuffers, offsets);
    m_device->m_disp.cmdBindIndexBuffer(m_cmd_bufs[image_index], m_index_buffer->get(), 0, VK_INDEX_TYPE_UINT16);
    m_device->m_disp.cmdDrawIndexed(m_cmd_bufs[image_index], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    m_imgui_renderer->render_draw_data(m_cmd_bufs[image_index]);

    m_device->m_disp.cmdEndRenderPass(m_cmd_bufs[image_index]);
    m_device->m_disp.endCommandBuffer(m_cmd_bufs[image_index]);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    const VkSemaphore waitSemaphores[] = { m_swapchain->get_available_semaphore() };
    constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_cmd_bufs[image_index];

    const VkSemaphore signalSemaphores[] = { m_swapchain->get_finished_semaphore(image_index) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    m_swapchain->reset_fence();

    if (m_device->m_disp.queueSubmit(m_device->get_graphics_queue(),
        1, &submitInfo, m_swapchain->get_in_flight_fence()) != VK_SUCCESS) {
        std::println("Failed to submit draw command buffer");
        return -1;
    }

    m_swapchain->present(image_index);

    return 0;
}

void App::start() {
    while (!m_window->should_close()) {
        m_window->poll_events();
        m_imgui_renderer->new_frame();

        ImGui::ShowDemoWindow();

        m_imgui_renderer->render();
        if (const int res = draw_frame(); res != 0) {
            return;
        }
    }
    m_device->m_disp.deviceWaitIdle();
}

void App::initialize_callbacks() {
    m_window->set_user_pointer(this);

    m_window->set_resize_callback([](GLFWwindow *window, int, int) {
        auto *app = static_cast<App*>(glfwGetWindowUserPointer(window));
        app->m_window_resized = true;
    });
}

} // Kynetic
