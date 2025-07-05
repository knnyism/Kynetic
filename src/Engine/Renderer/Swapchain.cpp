//
// Created by kenny on 3-7-25.
//

#include "Device.h"
#include "Swapchain.h"

namespace Kynetic {

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

Swapchain::Swapchain(const Device &device) : m_device(device) {
    create_swapchain();
    create_sync_objects();
}

Swapchain::~Swapchain() {
    for (size_t i = 0; i < m_swapchain.image_count; i++) {
        m_device.m_disp.destroySemaphore(m_finished_semaphores[i], nullptr);
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_device.m_disp.destroySemaphore(m_available_semaphores[i], nullptr);
        m_device.m_disp.destroyFence(m_in_flight_fences[i], nullptr);
    }

    m_swapchain.destroy_image_views(m_image_views);
    vkb::destroy_swapchain(m_swapchain);
}

void Swapchain::create_sync_objects() {
    m_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_finished_semaphores.resize(m_swapchain.image_count);
    m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    m_image_in_flight.resize(m_swapchain.image_count, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_swapchain.image_count; i++) {
        if (m_device.m_disp.createSemaphore(&semaphoreInfo, nullptr, &m_finished_semaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects");
        }
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_device.m_disp.createSemaphore(&semaphoreInfo, nullptr, &m_available_semaphores[i]) != VK_SUCCESS ||
            m_device.m_disp.createFence(&fenceInfo, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects");
            }
    }
}

void Swapchain::create_swapchain() {
    m_device.m_disp.deviceWaitIdle();

    vkb::SwapchainBuilder swapchain_builder(m_device.get_device());
    auto swapchain_result = swapchain_builder
        .set_old_swapchain(m_swapchain)
        .build();

    if (!swapchain_result) {
        std::println("{} {}", swapchain_result.error().message(), static_cast<int>(swapchain_result.vk_result()));
        throw std::runtime_error("Failed to create swapchain");
    }

    m_swapchain.destroy_image_views(m_image_views);
    vkb::destroy_swapchain(m_swapchain);

    m_swapchain = swapchain_result.value();

    auto images_result = m_swapchain.get_images();
    auto image_views_result = m_swapchain.get_image_views();

    if (!images_result || !image_views_result) {
        throw std::runtime_error("Failed to get swapchain images or image views");
    }

    m_images = images_result.value();
    m_image_views = image_views_result.value();
}

void Swapchain::reset_fence() const {
    m_device.m_disp.resetFences(1, &m_in_flight_fences[m_frame_index]);
}

uint32_t Swapchain::acquire_next_image_index() {
    m_device.m_disp.waitForFences(1, &m_in_flight_fences[m_frame_index], VK_TRUE, UINT64_MAX);

    uint32_t image_index;

    if (const VkResult result = m_device.m_disp.acquireNextImageKHR(m_swapchain.swapchain, UINT64_MAX,
                                                                    m_available_semaphores[m_frame_index],
                                                                    VK_NULL_HANDLE,
                                                                    &image_index); result == VK_ERROR_OUT_OF_DATE_KHR) {
        // create_swapchain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    if (m_image_in_flight[image_index] != VK_NULL_HANDLE) {
        m_device.m_disp.waitForFences(1, &m_image_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }
    m_image_in_flight[image_index] = m_in_flight_fences[m_frame_index];

    return image_index;
}

void Swapchain::present(uint32_t image_index) {
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_finished_semaphores[image_index],
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain.swapchain,
        .pImageIndices = &image_index
    };

    if (const VkResult result = m_device.m_disp.queuePresentKHR(m_device.get_present_queue(), &present_info);
        result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // create_swapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }

    m_frame_index = (m_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

} // Kynetic