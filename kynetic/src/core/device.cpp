//
// Created by kennypc on 11/4/25.
//

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"
#include "VkBootstrap.h"
#include "vk_mem_alloc.h"

#include "core/device.hpp"

#include "rendering/swapchain.hpp"

KX_DISABLE_WARNING_PUSH
KX_DISABLE_WARNING_OUTSIDE_RANGE
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
KX_DISABLE_WARNING_POP

using namespace kynetic;

Device::Device()
{
    SDL_Init(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow("Kynetic App",
                                static_cast<int>(m_window_extent.width),
                                static_cast<int>(m_window_extent.height),
                                SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

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

    VkPhysicalDeviceVulkan11Features features_11{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    features_11.shaderDrawParameters = true;

    VkPhysicalDeviceFeatures features{};
    features.shaderInt64 = true;

    vkb::PhysicalDeviceSelector selector{instance};
    vkb::PhysicalDevice physical_device = selector.set_minimum_version(1, 3)
                                              .set_required_features_11(features_11)
                                              .set_required_features_13(features_13)
                                              .set_required_features_12(features_12)
                                              .set_required_features(features)
                                              .set_surface(m_surface)
                                              .select()
                                              .value();
    m_physical_device = physical_device.physical_device;

    vkb::Device device = vkb::DeviceBuilder(physical_device).build().value();
    m_device = device.device;

    m_swapchain = std::make_unique<Swapchain>(m_device);
    m_swapchain->init(m_physical_device, m_surface, m_window_extent.width, m_window_extent.height);

    m_graphics_queue = device.get_queue(vkb::QueueType::graphics).value();
    m_present_queue = device.get_queue(vkb::QueueType::present).value();
    m_queue_indices = {.graphics = device.get_queue_index(vkb::QueueType::graphics).value(),
                       .present = device.get_queue_index(vkb::QueueType::present).value()};

    size_t swapchain_image_count = m_swapchain->m_images.size();
    m_syncs.resize(swapchain_image_count);

    VkSemaphoreCreateInfo semaphore_create_info = vk_init::semaphore_create_info();
    VkFenceCreateInfo fence_create_info = vk_init::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    for (size_t i = 0; i < swapchain_image_count; i++)
    {
        VK_CHECK(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_syncs[i].image_available));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_syncs[i].render_finished));
        VK_CHECK(vkCreateFence(m_device, &fence_create_info, nullptr, &m_syncs[i].in_flight_fence));
    }

    for (auto& ctx : m_ctxs) ctx.dcb.init(m_device, m_queue_indices.graphics);

    VkCommandPoolCreateInfo command_pool_info =
        vk_init::command_pool_create_info(m_queue_indices.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_immediate_command_pool));
    VkCommandBufferAllocateInfo cmdAllocInfo = vk_init::command_buffer_allocate_info(m_immediate_command_pool, 1);
    VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_immediate_command_buffer));
    VK_CHECK(vkCreateFence(m_device, &fence_create_info, nullptr, &m_immediate_fence));

    VmaAllocatorCreateInfo allocator_info = {.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                             .physicalDevice = m_physical_device,
                                             .device = m_device,
                                             .instance = m_instance};
    vmaCreateAllocator(&allocator_info, &m_allocator);

    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4},
    };
    m_descriptor_allocator.init_pool(m_device, 1000, sizes);

    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    VK_CHECK(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &imgui_descriptor_pool));

    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL3_InitForVulkan(m_window);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_physical_device;
    init_info.Device = m_device;
    init_info.Queue = m_graphics_queue;
    init_info.DescriptorPool = imgui_descriptor_pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.UseDynamicRendering = true;

    // dynamic rendering parameters for imgui to use
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapchain->m_image_format;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    createGlobalSession(m_slang_session.writeRef());
}

Device::~Device()
{
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(m_device, imgui_descriptor_pool, nullptr);

    m_descriptor_allocator.destroy_pool(m_device);

    vmaDestroyAllocator(m_allocator);

    vkDestroyFence(m_device, m_immediate_fence, nullptr);
    vkDestroyCommandPool(m_device, m_immediate_command_pool, nullptr);

    for (auto& ctx : m_ctxs)
    {
        ctx.dcb.shutdown();
        ctx.deletion_queue.flush();
    }

    for (auto sync : m_syncs)
    {
        vkDestroyFence(m_device, sync.in_flight_fence, nullptr);
        vkDestroySemaphore(m_device, sync.render_finished, nullptr);
        vkDestroySemaphore(m_device, sync.image_available, nullptr);
    }

    m_swapchain->shutdown();

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);

    vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
    vkDestroyInstance(m_instance, nullptr);
    SDL_DestroyWindow(m_window);
}

bool Device::begin_frame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    auto& ctx = get_context();
    uint32_t frame_index = m_frame_count % static_cast<uint32_t>(m_syncs.size());

    VK_CHECK(vkWaitForFences(m_device, 1, &m_syncs[frame_index].in_flight_fence, true, 1000000000));

    ctx.deletion_queue.flush();
    VkResult result = m_swapchain->acquire_next_image(m_syncs[frame_index].image_available);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_resize_requested = true;
        return false;
    }

    VK_CHECK(vkResetFences(m_device, 1, &m_syncs[frame_index].in_flight_fence));

    ctx.dcb.reset();

    const VkCommandBufferBeginInfo dcb_begin_info =
        vk_init::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(ctx.dcb.m_command_buffer, &dcb_begin_info));

    return true;
}

void Device::end_frame()
{
    ImGui::Render();

    const auto& ctx = get_context();
    uint32_t image_index = m_swapchain->m_image_index;
    uint32_t frame_index = m_frame_count % static_cast<uint32_t>(m_syncs.size());

    VkRenderingAttachmentInfo colorAttachment =
        vk_init::attachment_info(m_swapchain->get_image_view(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = vk_init::rendering_info(m_swapchain->m_extent, &colorAttachment, nullptr);

    vkCmdBeginRendering(ctx.dcb.m_command_buffer, &renderInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ctx.dcb.m_command_buffer);
    vkCmdEndRendering(ctx.dcb.m_command_buffer);

    ctx.dcb.transition_image(m_swapchain->get_video_out(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(ctx.dcb.m_command_buffer));

    VkCommandBufferSubmitInfo command_buffer_info = vk_init::command_buffer_submit_info(ctx.dcb.m_command_buffer);

    VkSemaphoreSubmitInfo wait_info = vk_init::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                                     m_syncs[frame_index].image_available);
    VkSemaphoreSubmitInfo signal_info =
        vk_init::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_syncs[image_index].render_finished);
    VkSubmitInfo2 submit = vk_init::submit_info(&command_buffer_info, &signal_info, &wait_info);

    VK_CHECK(vkQueueSubmit2(m_graphics_queue, 1, &submit, m_syncs[frame_index].in_flight_fence));

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_syncs[image_index].render_finished,
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain->m_swapchain,
        .pImageIndices = &m_swapchain->m_image_index,
    };

    VkResult result = vkQueuePresentKHR(m_graphics_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) m_resize_requested = true;

    m_frame_count++;
}

void Device::resize_swapchain()
{
    vkDeviceWaitIdle(m_device);

    m_swapchain->shutdown();

    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    m_window_extent.width = static_cast<uint32_t>(w);
    m_window_extent.height = static_cast<uint32_t>(h);

    m_swapchain->init(m_physical_device, m_surface, m_window_extent.width, m_window_extent.height);

    m_resize_requested = false;
}

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
            case SDL_EVENT_WINDOW_RESIZED:
                m_resize_requested = true;
                break;
            default:
                break;
        }

        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    if (m_resize_requested) resize_swapchain();
}

bool Device::is_minimized() const { return m_is_minimized; }

bool Device::is_running() const { return m_is_running; }

const VkImage& Device::get_video_out() const { return m_swapchain->get_video_out(); }

AllocatedImage Device::create_image(const VkExtent3D extent,
                                    const VkFormat format,
                                    const VmaMemoryUsage usage,
                                    const VkImageUsageFlags usage_flags,
                                    const VkMemoryPropertyFlags property_flags,
                                    const VkImageAspectFlags aspect_flags) const
{
    AllocatedImage image;

    image.extent = extent;
    image.format = format;
    const VkImageCreateInfo rimg_info = vk_init::image_create_info(image.format, usage_flags, extent);

    VmaAllocationCreateInfo rimg_alloc_info = {
        .usage = usage,
        .requiredFlags = property_flags,
    };
    vmaCreateImage(m_allocator, &rimg_info, &rimg_alloc_info, &image.image, &image.allocation, nullptr);

    const VkImageViewCreateInfo rview_info = vk_init::imageview_create_info(image.format, image.image, aspect_flags);

    VK_CHECK(vkCreateImageView(m_device, &rview_info, nullptr, &image.view));

    return image;
}

AllocatedImage Device::create_image(const VkExtent2D extent,
                                    const VkFormat format,
                                    const VmaMemoryUsage usage,
                                    const VkImageUsageFlags usage_flags,
                                    const VkMemoryPropertyFlags property_flags,
                                    const VkImageAspectFlags aspect_flags) const
{
    return create_image({.width = extent.width, .height = extent.height, .depth = 1},
                        format,
                        usage,
                        usage_flags,
                        property_flags,
                        aspect_flags);
}

void Device::destroy_image(const AllocatedImage& image) const
{
    vkDestroyImageView(m_device, image.view, nullptr);
    vmaDestroyImage(m_allocator, image.image, image.allocation);
}

AllocatedBuffer Device::create_buffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) const
{
    VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.pNext = nullptr;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = memory_usage;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocatedBuffer newBuffer;

    VK_CHECK(
        vmaCreateBuffer(m_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

    return newBuffer;
}

void Device::destroy_buffer(const AllocatedBuffer& buffer) const
{
    vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
}

void Device::wait_idle() const { vkDeviceWaitIdle(m_device); }

void Device::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) const
{
    VK_CHECK(vkResetFences(m_device, 1, &m_immediate_fence));
    VK_CHECK(vkResetCommandBuffer(m_immediate_command_buffer, 0));

    VkCommandBufferBeginInfo begin_info = vk_init::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(m_immediate_command_buffer, &begin_info));

    function(m_immediate_command_buffer);

    VK_CHECK(vkEndCommandBuffer(m_immediate_command_buffer));

    VkCommandBufferSubmitInfo submit_info = vk_init::command_buffer_submit_info(m_immediate_command_buffer);
    VkSubmitInfo2 submit = vk_init::submit_info(&submit_info, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(m_graphics_queue, 1, &submit, m_immediate_fence));
    VK_CHECK(vkWaitForFences(m_device, 1, &m_immediate_fence, true, 9999999999));
}
