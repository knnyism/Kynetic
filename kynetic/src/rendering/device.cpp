//
// Created by kennypc on 11/4/25.
//

#include "device.hpp"

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"

#include "VkBootstrap.h"
#include "swapchain.hpp"

using namespace kynetic;

Device::Device()
{
    SDL_Init(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow("Vulkan Engine",
                                static_cast<int>(m_window_extent.width),
                                static_cast<int>(m_window_extent.height),
                                SDL_WINDOW_VULKAN);

    auto instance = vkb::InstanceBuilder()
                        .set_app_name("Kynetic App")
                        .request_validation_layers(USE_VALIDATION_LAYERS)
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .build()
                        .value();
    m_instance = instance.instance;
    m_debug_messenger = instance.debug_messenger;

    SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface);

    VkPhysicalDeviceVulkan13Features features_13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features_13.dynamicRendering = true;
    features_13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features_12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features_12.bufferDeviceAddress = true;
    features_12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector{instance};
    vkb::PhysicalDevice physical_device = selector.set_minimum_version(1, 3)
                                              .set_required_features_13(features_13)
                                              .set_required_features_12(features_12)
                                              .set_surface(m_surface)
                                              .select()
                                              .value();
    m_physical_device = physical_device.physical_device;

    vkb::Device device = vkb::DeviceBuilder(physical_device).build().value();
    m_device = device.device;

    m_swapchain = new Swapchain(m_device, m_physical_device, m_surface, m_window_extent.width, m_window_extent.height);
}

Device::~Device()
{
    delete m_swapchain;

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);

    vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
    vkDestroyInstance(m_instance, nullptr);
    SDL_DestroyWindow(m_window);
}

void Device::begin_frame() const {}

void Device::end_frame() const {}

void Device::update()
{
    static SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                m_is_running = false;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                m_is_minimized = true;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
                m_is_minimized = false;
                break;
        }
    }
}

bool Device::is_minimized() const { return m_is_minimized; }

bool Device::is_running() const { return m_is_running; }
