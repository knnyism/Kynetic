//
// Created by kenny on 4-7-25.
//

#pragma once

namespace Kynetic {

class Device {
    vkb::Instance m_instance;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    vkb::Device m_device;

    VmaAllocator m_allocator = VK_NULL_HANDLE;

    VkQueue graphics_queue;
    VkQueue present_queue;

    void create_instance();
    void create_surface(GLFWwindow *window, const VkAllocationCallbacks *allocator);
    void create_device();
    void create_allocator();
public:
    vkb::InstanceDispatchTable m_instance_disp;
    vkb::DispatchTable m_disp;

    explicit Device(GLFWwindow *window, const VkAllocationCallbacks *allocator = nullptr);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    [[nodiscard]] const vkb::Instance& get_instance() const { return m_instance; }
    [[nodiscard]] VkSurfaceKHR get_surface() const { return m_surface; }
    [[nodiscard]] const vkb::Device& get_device() const { return m_device; }
    [[nodiscard]] VmaAllocator get_allocator() const { return m_allocator; }

    [[nodiscard]] VkQueue get_graphics_queue() const { return graphics_queue; }
    [[nodiscard]] VkQueue get_present_queue() const { return present_queue; }
};

} // Kynetic
