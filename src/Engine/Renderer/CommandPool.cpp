//
// Created by kenny on 6-7-25.
//

#include "Device.h"
#include "CommandPool.h"

namespace Kynetic {

CommandPool::CommandPool(const Device &device) : m_device(device) {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_device.get_device().get_queue_index(vkb::QueueType::graphics).value();

    if (m_device.m_disp.createCommandPool(&poolInfo, nullptr, &m_cmd_pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

CommandPool::CommandPool(CommandPool &&other) noexcept : m_device(other.m_device) {
    m_cmd_pool = std::exchange(other.m_cmd_pool, nullptr);
}

CommandPool::~CommandPool() {
    m_device.m_disp.destroyCommandPool(m_cmd_pool, nullptr);
}

std::vector<VkCommandBuffer> CommandPool::allocate_buffers(uint32_t count) const {
    std::vector<VkCommandBuffer> buffers(count);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_cmd_pool;
    allocInfo.commandBufferCount = static_cast<uint32_t>(buffers.size());

    if (m_device.m_disp.allocateCommandBuffers(&allocInfo, buffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    return buffers;
}

} // Kynetic