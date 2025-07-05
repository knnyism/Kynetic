//
// Created by kenny on 4-7-25.
//

#include "Device.h"

namespace Kynetic {

Device::Device(GLFWwindow *window, const VkAllocationCallbacks *allocator) {
    create_instance();
    create_surface(window, allocator);
    create_device();

    auto graphics_queue_result = m_device.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_result.has_value()) {
        std::println("{}", graphics_queue_result.error().message());
        throw std::runtime_error("Failed to get graphics queue");
    }
    graphics_queue = graphics_queue_result.value();

    auto present_queue_result = m_device.get_queue(vkb::QueueType::present);
    if (!present_queue_result.has_value()) {
        std::println("{}", present_queue_result.error().message());
        throw std::runtime_error("Failed to get present queue");
    }
    present_queue = present_queue_result.value();
}

Device::~Device() {
    vkb::destroy_device(m_device);
    vkb::destroy_surface(m_instance, m_surface);
    vkb::destroy_instance(m_instance);
}

void Device::create_instance() {
    vkb::InstanceBuilder instance_builder;
    auto instance_result = instance_builder.use_default_debug_messenger().request_validation_layers().build();
    if (!instance_result) {
        std::println("{}", instance_result.error().message());
        throw std::runtime_error("Failed to create instance");
    }

    m_instance = instance_result.value();
    m_instance_disp = m_instance.make_table();
}

void Device::create_surface(GLFWwindow *window, const VkAllocationCallbacks *allocator) {
    if (glfwCreateWindowSurface(m_instance, window, allocator, &m_surface) != VK_SUCCESS) {
        const char* error_message;
        if (int result = glfwGetError(&error_message); result != 0) {
            std::println("{} {}", result, error_message);
            throw std::runtime_error("Failed to create surface");
        }
    }
}

void Device::create_device() {
    vkb::PhysicalDeviceSelector physical_device_selector(m_instance);
    auto physical_device_result = physical_device_selector.set_surface(m_surface).select();
    if (!physical_device_result) {
        std::println("{}", physical_device_result.error().message());
        throw std::runtime_error("Failed to select physical device");
    }

    const vkb::DeviceBuilder device_builder(physical_device_result.value());
    auto device_result = device_builder.build();
    if (!device_result) {
        std::println("{}", device_result.error().message());
        throw std::runtime_error("Failed to create device");
    }

    m_device = device_result.value();
    m_disp = m_device.make_table();
}

} // Kynetic