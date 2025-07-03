//
// Created by kenny on 7/1/25.
//

#include "Engine/App.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

GLFWwindow* Kynetic::App::CreateWindowGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    return glfwCreateWindow(1024, 1024, "Kynetic", NULL, NULL);
}

void Kynetic::App::DestroyWindowGlfw() const {
    glfwDestroyWindow(window);
    glfwTerminate();
}

VkSurfaceKHR Kynetic::App::CreateSurfaceGlfw(VkInstance instance, GLFWwindow* window, VkAllocationCallbacks* allocator = nullptr) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult err = glfwCreateWindowSurface(instance, window, allocator, &surface);
    if (err) {
        const char* error_msg;
        int ret = glfwGetError(&error_msg);
        if (ret != 0) {
        }
        surface = VK_NULL_HANDLE;
    }
    return surface;
}

int Kynetic::App::InitializeDeviceVulkan() {
    window = CreateWindowGlfw();

    vkb::InstanceBuilder instance_builder;
    auto instance_ret = instance_builder.use_default_debug_messenger().request_validation_layers().build();
    if (!instance_ret) {
        return -1;
    }
    instance = instance_ret.value();

    inst_disp = instance.make_table();

    surface = CreateSurfaceGlfw(instance, window);

    vkb::PhysicalDeviceSelector phys_device_selector(instance);
    auto phys_device_ret = phys_device_selector.set_surface(surface).select();
    if (!phys_device_ret) {
        return -1;
    }
    vkb::PhysicalDevice physical_device = phys_device_ret.value();

    vkb::DeviceBuilder device_builder{ physical_device };
    auto device_ret = device_builder.build();
    if (!device_ret) {
        return -1;
    }
    device = device_ret.value();

    disp = device.make_table();

    return 0;
}

int Kynetic::App::CreateSwapchainVulkan() {
    vkb::SwapchainBuilder swapchain_builder{ device };
    auto swap_ret = swapchain_builder.set_old_swapchain(swapchain).build();
    if (!swap_ret) {
        return -1;
    }
    vkb::destroy_swapchain(swapchain);
    swapchain = swap_ret.value();
    return 0;
}

int Kynetic::App::GetQueuesVulkan(RenderData& data) const {
    auto gq = device.get_queue(vkb::QueueType::graphics);
    if (!gq.has_value()) {
        return -1;
    }
    data.graphics_queue = gq.value();

    auto pq = device.get_queue(vkb::QueueType::present);
    if (!pq.has_value()) {
        return -1;
    }
    data.present_queue = pq.value();
    return 0;
}

int Kynetic::App::CreateRenderPassVulkan(RenderData& data) const {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (disp.createRenderPass(&render_pass_info, nullptr, &data.render_pass) != VK_SUCCESS) {
        return -1; // failed to create render pass!
    }
    return 0;
}

VkShaderModule Kynetic::App::CreateShaderModuleVulkan(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (disp.createShaderModule(&create_info, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE; // failed to create shader module
    }

    return shaderModule;
}

int Kynetic::App::CreateGraphicsPipelineVulkan(RenderData& data) const {
    auto vert_code = ReadFile(std::string(KYNETIC_SOURCE_DIR) + "/Assets/Shaders/triangle.vert.spv");
    auto frag_code = ReadFile(std::string(KYNETIC_SOURCE_DIR) + "/Assets/Shaders/triangle.frag.spv");

     VkShaderModule vert_module = CreateShaderModuleVulkan(vert_code);
    VkShaderModule frag_module = CreateShaderModuleVulkan(frag_code);
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
        return -1; // failed to create shader modules
    }

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_module;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain.extent.width;
    viewport.height = (float)swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &colorBlendAttachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (disp.createPipelineLayout(&pipeline_layout_info, nullptr, &data.pipeline_layout) != VK_SUCCESS) {
        return -1; // failed to create pipeline layout
    }

    std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = data.pipeline_layout;
    pipeline_info.renderPass = data.render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &data.graphics_pipeline) != VK_SUCCESS) {
        return -1;
    }

    disp.destroyShaderModule(frag_module, nullptr);
    disp.destroyShaderModule(vert_module, nullptr);
    return 0;
}

int Kynetic::App::CreateFramebuffersVulkan(RenderData& data) {
    data.swapchain_images = swapchain.get_images().value();
    data.swapchain_image_views = swapchain.get_image_views().value();

    data.framebuffers.resize(data.swapchain_image_views.size());

    for (size_t i = 0; i < data.swapchain_image_views.size(); i++) {
        VkImageView attachments[] = { data.swapchain_image_views[i] };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = data.render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swapchain.extent.width;
        framebuffer_info.height = swapchain.extent.height;
        framebuffer_info.layers = 1;

        if (disp.createFramebuffer(&framebuffer_info, nullptr, &data.framebuffers[i]) != VK_SUCCESS) {
            return -1; // failed to create framebuffer
        }
    }
    return 0;
}

int Kynetic::App::CreateCommandPoolVulkan(RenderData& data) const {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();

    if (disp.createCommandPool(&pool_info, nullptr, &data.command_pool) != VK_SUCCESS) {
        return -1; // failed to create command pool
    }
    return 0;
}

int Kynetic::App::CreateCommandBuffersVulkan(RenderData& data) const {
    data.command_buffers.resize(data.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = data.command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)data.command_buffers.size();

    if (disp.allocateCommandBuffers(&allocInfo, data.command_buffers.data()) != VK_SUCCESS) {
        return -1; // failed to allocate command buffers;
    }

    for (size_t i = 0; i < data.command_buffers.size(); i++) {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (disp.beginCommandBuffer(data.command_buffers[i], &begin_info) != VK_SUCCESS) {
            return -1; // failed to begin recording command buffer
        }

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = data.render_pass;
        render_pass_info.framebuffer = data.framebuffers[i];
        render_pass_info.renderArea.offset = { 0, 0 };
        render_pass_info.renderArea.extent = swapchain.extent;
        VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchain.extent.width;
        viewport.height = (float)swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = swapchain.extent;

        disp.cmdSetViewport(data.command_buffers[i], 0, 1, &viewport);
        disp.cmdSetScissor(data.command_buffers[i], 0, 1, &scissor);

        disp.cmdBeginRenderPass(data.command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        disp.cmdBindPipeline(data.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, data.graphics_pipeline);

        disp.cmdDraw(data.command_buffers[i], 3, 1, 0, 0);

        disp.cmdEndRenderPass(data.command_buffers[i]);

        if (disp.endCommandBuffer(data.command_buffers[i]) != VK_SUCCESS) {
            return -1; // failed to record command buffer!
        }
    }
    return 0;
}

int Kynetic::App::CreateSyncObjectsVulkan(RenderData& data) const {
    data.available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    data.finished_semaphore.resize(swapchain.image_count);
    data.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
    data.image_in_flight.resize(swapchain.image_count, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < swapchain.image_count; i++) {
        if (disp.createSemaphore(&semaphore_info, nullptr, &data.finished_semaphore[i]) != VK_SUCCESS) {
            return -1; // failed to create synchronization objects for a frame
        }
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (disp.createSemaphore(&semaphore_info, nullptr, &data.available_semaphores[i]) != VK_SUCCESS ||
            disp.createFence(&fence_info, nullptr, &data.in_flight_fences[i]) != VK_SUCCESS) {
            return -1; // failed to create synchronization objects for a frame
            }
    }
    return 0;
}

int Kynetic::App::RecreateSwapchainVulkan(RenderData& data) {
    disp.deviceWaitIdle();

    disp.destroyCommandPool(data.command_pool, nullptr);

    for (auto framebuffer : data.framebuffers) {
        disp.destroyFramebuffer(framebuffer, nullptr);
    }

    swapchain.destroy_image_views(data.swapchain_image_views);

    if (0 != CreateSwapchainVulkan()) return -1;
    if (0 != CreateFramebuffersVulkan(data)) return -1;
    if (0 != CreateCommandPoolVulkan(data)) return -1;
    if (0 != CreateCommandBuffersVulkan(data)) return -1;

    return 0;
}

int Kynetic::App::DrawFrame(RenderData& data) {
    disp.waitForFences(1, &data.in_flight_fences[data.current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult result = disp.acquireNextImageKHR(
        swapchain, UINT64_MAX, data.available_semaphores[data.current_frame], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return RecreateSwapchainVulkan(data);
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return -1;
    }

    if (data.image_in_flight[image_index] != VK_NULL_HANDLE) {
        disp.waitForFences(1, &data.image_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }
    data.image_in_flight[image_index] = data.in_flight_fences[data.current_frame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = { data.available_semaphores[data.current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &data.command_buffers[image_index];

    VkSemaphore signal_semaphores[] = { data.finished_semaphore[image_index] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    disp.resetFences(1, &data.in_flight_fences[data.current_frame]);

    if (disp.queueSubmit(data.graphics_queue, 1, &submitInfo, data.in_flight_fences[data.current_frame]) != VK_SUCCESS) {
        return -1; //"failed to submit draw command buffer
    }

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapChains[] = { swapchain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapChains;

    present_info.pImageIndices = &image_index;

    result = disp.queuePresentKHR(data.present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return RecreateSwapchainVulkan(data);
    } else if (result != VK_SUCCESS) {
        return -1;
    }

    data.current_frame = (data.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
    return 0;
}

int Kynetic::App::InitializeVulkan(RenderData& data) {
    if (0 != InitializeDeviceVulkan()) return -1;
    if (0 != CreateSwapchainVulkan()) return -1;
    if (0 != GetQueuesVulkan(data)) return -1;
    if (0 != CreateRenderPassVulkan(data)) return -1;
    if (0 != CreateGraphicsPipelineVulkan(data)) return -1;
    if (0 != CreateFramebuffersVulkan(data)) return -1;
    if (0 != CreateCommandPoolVulkan(data)) return -1;
    if (0 != CreateCommandBuffersVulkan(data)) return -1;
    if (0 != CreateSyncObjectsVulkan(data)) return -1;

    return 0;
}

void Kynetic::App::Cleanup(RenderData& data) {
    for (size_t i = 0; i < swapchain.image_count; i++) {
        disp.destroySemaphore(data.finished_semaphore[i], nullptr);
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        disp.destroySemaphore(data.available_semaphores[i], nullptr);
        disp.destroyFence(data.in_flight_fences[i], nullptr);
    }

    disp.destroyCommandPool(data.command_pool, nullptr);

    for (auto framebuffer : data.framebuffers) {
        disp.destroyFramebuffer(framebuffer, nullptr);
    }

    disp.destroyPipeline(data.graphics_pipeline, nullptr);
    disp.destroyPipelineLayout(data.pipeline_layout, nullptr);
    disp.destroyRenderPass(data.render_pass, nullptr);

    swapchain.destroy_image_views(data.swapchain_image_views);

    vkb::destroy_swapchain(swapchain);
    vkb::destroy_device(device);
    vkb::destroy_surface(instance, surface);
    vkb::destroy_instance(instance);
    DestroyWindowGlfw();
}

void Kynetic::App::Start() {
    RenderData data;

    InitializeVulkan(data);
    InitializeImGui();
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        int res = DrawFrame(data);
        if (res != 0) {
            return;
        }
    }
    disp.deviceWaitIdle();

    Cleanup(data);
}

void Kynetic::App::InitializeImGui() {
}
