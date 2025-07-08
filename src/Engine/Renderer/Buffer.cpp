//
// Created by kenny on 7-7-25.
//

#include "Device.h"
#include "Buffer.h"

namespace Kynetic {

VkBufferUsageFlags Buffer::get_usage_flags(const BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferUsage::Index:
            return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case BufferUsage::Uniform:
            return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        case BufferUsage::Storage:
            return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        case BufferUsage::Staging:
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        default:
            throw std::runtime_error("Unknown buffer usage type");
    }
}

VmaMemoryUsage Buffer::get_memory_usage(const BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex:
        case BufferUsage::Index:
            return VMA_MEMORY_USAGE_GPU_ONLY;
        case BufferUsage::Uniform:
            return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case BufferUsage::Storage:
            return VMA_MEMORY_USAGE_GPU_ONLY;
        case BufferUsage::Staging:
            return VMA_MEMORY_USAGE_CPU_ONLY;
        default:
            throw std::runtime_error("Unknown buffer usage type");
    }
}

Buffer::Buffer(const Device& device, VkDeviceSize size, BufferUsage usage)
        : m_device(device), m_size(size), m_usage(usage) {

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = get_usage_flags(usage);
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = get_memory_usage(usage);

    if (usage == BufferUsage::Uniform) {
        alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    if (vmaCreateBuffer(device.get_allocator(), &buffer_info, &alloc_info, &m_buffer, &m_allocation, nullptr) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }

    if (usage == BufferUsage::Uniform) {
        VmaAllocationInfo allocation_info;
        vmaGetAllocationInfo(device.get_allocator(), m_allocation, &allocation_info);
        m_mapped_data = allocation_info.pMappedData;
        m_mapped = true;
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : m_device(other.m_device), m_size(other.m_size), m_usage(other.m_usage), m_mapped(other.m_mapped) {
    m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
    m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
    m_mapped_data = std::exchange(other.m_mapped_data, nullptr);
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        if (m_buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_device.get_allocator(), m_buffer, m_allocation);
        }

        m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, VK_NULL_HANDLE);
        m_size = other.m_size;
        m_usage = other.m_usage;
        m_mapped = other.m_mapped;
        m_mapped_data = std::exchange(other.m_mapped_data, nullptr);
    }
    return *this;
}

Buffer::~Buffer() {
    if (m_buffer != VK_NULL_HANDLE) {
        if (m_mapped && m_usage != BufferUsage::Uniform) {
            vmaUnmapMemory(m_device.get_allocator(), m_allocation);
        }
        vmaDestroyBuffer(m_device.get_allocator(), m_buffer, m_allocation);
    }
}

void Buffer::copy_data(const void* data, const VkDeviceSize size, const VkDeviceSize offset) {
    if (size + offset > m_size) {
        throw std::runtime_error("Data size exceeds buffer size");
    }

    if (m_usage == BufferUsage::Uniform && m_mapped_data) {
        std::memcpy(static_cast<char*>(m_mapped_data) + offset, data, size);
    } else {
        void* mapped = map();
        std::memcpy(static_cast<char*>(mapped) + offset, data, size);
        unmap();
    }
}

void* Buffer::map() {
    if (m_mapped) {
        return m_mapped_data;
    }

    if (vmaMapMemory(m_device.get_allocator(), m_allocation, &m_mapped_data) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map buffer memory");
    }

    m_mapped = true;
    return m_mapped_data;
}

void Buffer::unmap() {
    if (m_mapped && m_usage != BufferUsage::Uniform) {
        vmaUnmapMemory(m_device.get_allocator(), m_allocation);
        m_mapped = false;
        m_mapped_data = nullptr;
    }
}

void Buffer::copy_from_staging(const Buffer& staging_buffer, const VkCommandBuffer cmd_buffer) const {
    copy_buffer_to_buffer(cmd_buffer, staging_buffer.get(), m_buffer, m_size);
}

void Buffer::copy_buffer_to_buffer(const VkCommandBuffer cmd_buffer, const VkBuffer src, const VkBuffer dst,
                                   const VkDeviceSize size) {
    VkBufferCopy copy_region = {};
    copy_region.size = size;
    vkCmdCopyBuffer(cmd_buffer, src, dst, 1, &copy_region);
}

} // Kynetic