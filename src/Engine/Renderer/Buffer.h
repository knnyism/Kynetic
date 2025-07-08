//
// Created by kenny on 7-7-25.
//

#pragma once

namespace Kynetic {

class Device;

enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Storage,
    Staging
};

class Buffer {
    const Device& m_device;

    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    BufferUsage m_usage;

    bool m_mapped = false;
    void* m_mapped_data = nullptr;

    static VkBufferUsageFlags get_usage_flags(BufferUsage usage);

    static VmaMemoryUsage get_memory_usage(BufferUsage usage);
public:
    Buffer(const Device& device, VkDeviceSize size, BufferUsage usage);

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    ~Buffer();

    void copy_data(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    void* map();
    void unmap();

    void copy_from_staging(const Buffer& staging_buffer, VkCommandBuffer cmd_buffer) const;
    static void copy_buffer_to_buffer(VkCommandBuffer cmd_buffer, VkBuffer src, VkBuffer dst, VkDeviceSize size);

    [[nodiscard]] VkBuffer get() const { return m_buffer; }
    [[nodiscard]] VkDeviceSize size() const { return m_size; }
    [[nodiscard]] BufferUsage usage() const { return m_usage; }
    [[nodiscard]] bool is_mapped() const { return m_mapped; }
    [[nodiscard]] void* mapped_data() const { return m_mapped_data; }
};

} // Kynetic
