//
// Created by kenny on 6-7-25.
//

#pragma once

namespace Kynetic {

class Device;

class CommandPool {
    const Device &m_device;
    VkCommandPool m_cmd_pool = VK_NULL_HANDLE;
public:
    explicit CommandPool(const Device &device);

    CommandPool(const CommandPool &) = delete;
    CommandPool(CommandPool &&) noexcept;

    ~CommandPool();

    CommandPool &operator=(const CommandPool &) = delete;
    CommandPool &operator=(CommandPool &&) = delete;

    [[nodiscard]] VkCommandPool get() const { return m_cmd_pool; }

    [[nodiscard]] std::vector<VkCommandBuffer> allocate_buffers(uint32_t count) const;
};
} // Kynetic
