#include "vulkan_engine.h"

#define VOLK_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

bool updateSwapchain{ false };
glm::vec3 camPos{ 0.0f, 0.0f, -6.0f };
glm::vec3 objectRotations[3]{};

static inline auto chk(VkResult result) -> void
{
    if (result != VK_SUCCESS)
    {
        //LOG_ERROR("Vulkan call returned an error ({})", (int)result);
        // TODO: return error
        exit(result);
    }
}

static inline auto chkSwapchain(VkResult result) -> void
{
    if (result < VK_SUCCESS)
    {
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            updateSwapchain = true;
            return;
        }
        //LOG_ERROR("Vulkan call returned an error ({})", (int)result);
        // TODO: return error
        exit(result);
    }
}

auto VulkanEngine::init(u32 extensionsCount, const char* const* requiredExtensions) -> void
{
    volkInitialize();
    // Instance
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Solitaire",
        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = extensionsCount,
        .ppEnabledExtensionNames = requiredExtensions,
    };
    chk(vkCreateInstance(&instanceCI, nullptr, &m_instance));
    volkLoadInstance(m_instance);

    // Device
    u32 deviceCount{ 0 };
    chk(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr));
    std::vector<VkPhysicalDevice> devices(deviceCount);
    chk(vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()));
    u32 deviceIndex{ 0 };
    VkPhysicalDeviceProperties2 deviceProperties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
    };
    vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
    //LOG_INFO("Selected device: {}", deviceProperties.properties.deviceName);
    m_physicalDevice = devices[deviceIndex];

    // Find a queue family for graphics
    u32 queueFamilyCount{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties2(m_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties2> queueFamilies(queueFamilyCount);
    for (u64 i = 0; i < queueFamilies.size(); ++i)
    {
        queueFamilies[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(m_physicalDevice, &queueFamilyCount, queueFamilies.data());
    u32 queueFamily{ 0 };
    for (u64 i = 0; i < queueFamilies.size(); ++i)
    {
        if (queueFamilies[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queueFamily = i;
            break;
        }
    }
    // TODO: call this function
    //chk(SDL_Vulkan_GetPresentationSupport(m_instance, devices[deviceIndex], queueFamily));

    // Logical device
    const float qfpriorities{ 1.0f };
    VkDeviceQueueCreateInfo queueCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &qfpriorities
    };
    VkPhysicalDeviceVulkan12Features enabledVk12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true
    };
    VkPhysicalDeviceVulkan13Features enabledVk13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabledVk12Features,
        .synchronization2 = true,
        .dynamicRendering = true
    };
    VkPhysicalDeviceFeatures2 enabledVk10Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .features = {
            .samplerAnisotropy = VK_TRUE
        }
    };
    const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo deviceCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledVk13Features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCI,
        .enabledExtensionCount = static_cast<u32>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &enabledVk10Features.features
    };
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &enabledVk10Features);
    chk(vkCreateDevice(m_physicalDevice, &deviceCI, nullptr, &m_device));
    vkGetDeviceQueue(m_device, queueFamily, 0, &m_queue);

    // VMA
    VmaVulkanFunctions vkFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };
    VmaAllocatorCreateInfo allocatorCI{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_physicalDevice,
        .device = m_device,
        .pVulkanFunctions = &vkFunctions,
        .instance = m_instance
    };
    chk(vmaCreateAllocator(&allocatorCI, &m_allocator));

    // Command pool
    VkCommandPoolCreateInfo commandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamily
    };
    chk(vkCreateCommandPool(m_device, &commandPoolCI, nullptr, &m_commandPool));
    VkCommandBufferAllocateInfo cbAllocCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_commandPool,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT 
    };
    chk(vkAllocateCommandBuffers(m_device, &cbAllocCI, m_commandBuffers.data()));


    createImguiDescriptorPool();
}

auto VulkanEngine::setUniformData(GameObjectID id, void* data, u64 size) -> void
{
    m_gameObjects[(u64) id].shaderData.data = data;
    m_gameObjects[(u64) id].shaderData.size = size;
}

auto VulkanEngine::render() -> void
{
    // Sync
    chk(vkWaitForFences(m_device, 1, &m_fences[m_frameIndex], true, UINT64_MAX));
    chk(vkResetFences(m_device, 1, &m_fences[m_frameIndex]));
    chkSwapchain(vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_presentSemaphores[m_frameIndex], VK_NULL_HANDLE, &m_imageIndex));

    for (GameObject& go : m_gameObjects)
    {
        memcpy(go.shaderDataBuffers[m_frameIndex].allocationInfo.pMappedData, go.shaderData.data, go.shaderData.size);
    }

    // Build command buffer
    auto cb = m_commandBuffers[m_frameIndex];
    chk(vkResetCommandBuffer(cb, 0));
    VkCommandBufferBeginInfo cbBI {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    chk(vkBeginCommandBuffer(cb, &cbBI));
    std::array<VkImageMemoryBarrier2, 2> outputBarriers{
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = m_swapchainImages[m_imageIndex],
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        },
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = m_depthImage,
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        }
    };
    VkDependencyInfo barrierDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers = outputBarriers.data()
    };
    vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);
    VkRenderingAttachmentInfo colorAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_swapchainImageViews[m_imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue{ .color{ { 0.0f, 0.0f, 0.0f, 1.0f } } }
    };
    VkRenderingAttachmentInfo depthAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = { .depthStencil = { 1.0f, 0 } }
    };
    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea{
            .extent{
                .width = static_cast<u32>(m_windowSize.x),
                .height = static_cast<u32>(m_windowSize.y)
            }
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo
    };
    vkCmdBeginRendering(cb, &renderingInfo);
    VkViewport vp{
        .width = static_cast<float>(m_windowSize.x),
        .height = static_cast<float>(m_windowSize.y),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{
        .extent{
            .width = static_cast<u32>(m_windowSize.x),
            .height = static_cast<u32>(m_windowSize.y)
        }
    };

    for (GameObject& go : m_gameObjects)
    {
        auto& pipeline = m_pipelines[(u64) go.pipelineID];
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
        vkCmdSetScissor(cb, 0, 1, &scissor);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, 1, &m_descriptorSetTex, 0, nullptr);
        MeshBuffer& meshBuffer = m_meshBuffers[(u64) go.meshID];

        VkDeviceSize vOffset{ 0 };
        vkCmdBindVertexBuffers(cb, 0, 1, &meshBuffer.buffer, &vOffset);
        vkCmdBindIndexBuffer(cb, meshBuffer.buffer, meshBuffer.bufferSize, VK_INDEX_TYPE_UINT16);
        vkCmdPushConstants(cb, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &go.shaderDataBuffers[m_frameIndex].deviceAddress);
        vkCmdDrawIndexed(cb, meshBuffer.indexCount, go.instanceCount, 0, 0, 0);
    }
    //ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
    vkCmdEndRendering(cb);
    VkImageMemoryBarrier2 barrierPresent{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = m_swapchainImages[m_imageIndex],
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    VkDependencyInfo barrierPresentDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1, 
        .pImageMemoryBarriers = &barrierPresent
    };
    vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);
    chk(vkEndCommandBuffer(cb));

    // Submit to graphics queue
    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_presentSemaphores[m_frameIndex],
        .pWaitDstStageMask = &waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cb,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_renderSemaphores[m_imageIndex],
    };
    chk(vkQueueSubmit(m_queue, 1, &submitInfo, m_fences[m_frameIndex]));
    m_frameIndex = (m_frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderSemaphores[m_imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &m_imageIndex
    };
    chkSwapchain(vkQueuePresentKHR(m_queue, &presentInfo));

    if (updateSwapchain) {
        // TODO: fix this
        //glfwGetFramebufferSize(m_window, &m_windowSize.x, &m_windowSize.y);
        updateSwapchain = false;
        chk(vkDeviceWaitIdle(m_device));

        createSwapchain();
    }
}

auto VulkanEngine::terminate() -> void
{
    chk(vkDeviceWaitIdle(m_device));
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyFence(m_device, m_fences[i], nullptr);
        vkDestroySemaphore(m_device, m_presentSemaphores[i], nullptr);
        for (GameObject& go : m_gameObjects)
        {
            vmaDestroyBuffer(m_allocator, go.shaderDataBuffers[i].buffer, go.shaderDataBuffers[i].allocation);
        }
    }
    for (u64 i = 0; i < m_renderSemaphores.size(); ++i)
    {
        vkDestroySemaphore(m_device, m_renderSemaphores[i], nullptr);
    }
    vmaDestroyImage(m_allocator, m_depthImage, m_depthImageAllocation);
    vkDestroyImageView(m_device, m_depthImageView, nullptr);
    for (u64 i = 0; i < m_swapchainImageViews.size(); ++i)
    {
        vkDestroyImageView(m_device, m_swapchainImageViews[i], nullptr);
    }
    for (auto& meshBuffer : m_meshBuffers)
    {
        vmaDestroyBuffer(m_allocator, meshBuffer.buffer, meshBuffer.allocation);
    }
    for (u64 i = 0; i < m_textures.size(); ++i)
    {
        vkDestroyImageView(m_device, m_textures[i].view, nullptr);
        vkDestroySampler(m_device, m_textures[i].sampler, nullptr);
        vmaDestroyImage(m_allocator, m_textures[i].image, m_textures[i].allocation);
    }
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutTex, nullptr);
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    vkDestroyDescriptorPool(m_device, m_imguiPool, nullptr);
    for (auto& pipeline : m_pipelines)
    {
        vkDestroyPipelineLayout(m_device, pipeline.layout, nullptr);
        vkDestroyPipeline(m_device, pipeline.pipeline, nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    for (auto& shader : m_shaderModules)
    {
        vkDestroyShaderModule(m_device, shader, nullptr);
    }
    vmaDestroyAllocator(m_allocator);
    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

auto VulkanEngine::createImguiDescriptorPool() -> void
{
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (u32)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    chk(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_imguiPool));
}
/*
auto VulkanEngine::loadMeshData(std::vector<Vertex>& vertices, std::vector<u16>& indices) -> MeshID
{
    m_meshBuffers.push_back(MeshBuffer{});
    MeshBuffer& newMeshBuffer = m_meshBuffers.back();

    newMeshBuffer.bufferSize = sizeof(Vertex) * vertices.size();
    VkDeviceSize iBufSize{ sizeof(u16) * indices.size() };
    newMeshBuffer.indexCount = indices.size();
    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = newMeshBuffer.bufferSize + iBufSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };
    VmaAllocationCreateInfo vBufferAllocCI{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    VmaAllocationInfo vBufferAllocInfo{};
    chk(vmaCreateBuffer(m_allocator, &bufferCI, &vBufferAllocCI, &newMeshBuffer.buffer, &newMeshBuffer.allocation, &vBufferAllocInfo));
    memcpy(vBufferAllocInfo.pMappedData, vertices.data(), newMeshBuffer.bufferSize);
    memcpy(((char*)vBufferAllocInfo.pMappedData) + newMeshBuffer.bufferSize, indices.data(), iBufSize);

    return MeshID(m_meshBuffers.size() - 1);
}
*/
auto VulkanEngine::setAlphaBlendAttachment() -> VkPipelineColorBlendAttachmentState
{
    return {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = 0xF,
    };
}

auto VulkanEngine::createUniformBuffers() -> void
{
    for (GameObject& go : m_gameObjects)
    {
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VkBufferCreateInfo uBufferCI{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = go.shaderData.size,
                .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            };
            VmaAllocationCreateInfo uBufferAllocCI{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                    | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
                    | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO
            };
            chk(vmaCreateBuffer(m_allocator, &uBufferCI, &uBufferAllocCI, &go.shaderDataBuffers[i].buffer, &go.shaderDataBuffers[i].allocation, &go.shaderDataBuffers[i].allocationInfo));
            VkBufferDeviceAddressInfo uBufferBdaInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = go.shaderDataBuffers[i].buffer
            };
            go.shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(m_device, &uBufferBdaInfo);
        }
    }
}

auto VulkanEngine::addGameObject(MeshID id, PipelineID pipelineID) -> GameObjectID
{
    GameObject go;
    go.meshID = id;
    go.pipelineID = pipelineID;
    go.instanceCount = 1;
    m_gameObjects.push_back(go);

    return GameObjectID(m_gameObjects.size() - 1);
}


auto VulkanEngine::updateGameObjectInstanceCount(GameObjectID id, u64 instanceCount) -> void
{
    m_gameObjects[(u64) id].instanceCount = instanceCount;
}

auto VulkanEngine::getVulkanPointers() -> VulkanPointers
{
    return {
        .instance = m_instance,
        .physicalDevice = m_physicalDevice,
        .device = m_device,
        .queue = m_queue,
        .descriptorPool = m_imguiPool,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM
    };
}

auto VulkanEngine::waitDevice() -> void
{
    chk(vkDeviceWaitIdle(m_device));
}

auto VulkanEngine::setSurfaceAndWindowSize(VkSurfaceKHR surface, u32 sizeX, u32 sizeY) -> void
{
    m_surface = surface;
    m_windowSize.x = sizeX;
    m_windowSize.y = sizeY;
}

auto VulkanEngine::createSwapchain() -> void
{
    VkSurfaceCapabilitiesKHR surfaceCaps{};
    chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCaps));
    VkExtent2D swapchainExtent{ surfaceCaps.currentExtent };
    if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
        swapchainExtent = {
            .width = static_cast<u32>(m_windowSize.x),
            .height = static_cast<u32>(m_windowSize.y)
        };
    }

    const VkFormat imageFormat{ VK_FORMAT_B8G8R8A8_UNORM};
    VkSwapchainCreateInfoKHR swapchainCI{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .minImageCount = surfaceCaps.minImageCount,
        .imageFormat = imageFormat,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent{ .width = swapchainExtent.width, .height = swapchainExtent.height },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };
    chk(vkCreateSwapchainKHR(m_device, &swapchainCI, nullptr, &m_swapchain));
    chk(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr));
    m_swapchainImages.resize(m_imageCount);
    chk(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, m_swapchainImages.data()));
    m_swapchainImageViews.resize(m_imageCount);
    for (u32 i = 0; i < m_imageCount; ++i)
    {
        VkImageViewCreateInfo viewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageFormat,
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };
        chk(vkCreateImageView(m_device, &viewCI, nullptr, &m_swapchainImageViews[i]));
    }

    // Depth attachment
    std::vector<VkFormat> depthFormatList{
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
    for (VkFormat& format : depthFormatList)
    {
        VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
        vkGetPhysicalDeviceFormatProperties2(m_physicalDevice, format, &formatProperties);
        if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            depthFormat = format;
            break;
        }
    }
    assert(depthFormat != VK_FORMAT_UNDEFINED);
    VkImageCreateInfo depthImageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent{
            .width = static_cast<u32>(m_windowSize.x),
            .height = static_cast<u32>(m_windowSize.y),
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    chk(vmaCreateImage(m_allocator, &depthImageCI, &allocCI, &m_depthImage, &m_depthImageAllocation, nullptr));
    VkImageViewCreateInfo depthViewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    chk(vkCreateImageView(m_device, &depthViewCI, nullptr, &m_depthImageView));
}

auto VulkanEngine::instance() -> VkInstance
{
    return m_instance;
}

auto VulkanEngine::createSyncObjects() -> void
{
    VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        chk(vkCreateFence(m_device, &fenceCI, nullptr, &m_fences[i]));
        chk(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_presentSemaphores[i]));
    }
    m_renderSemaphores.resize(m_swapchainImages.size());
    for (auto& semaphore : m_renderSemaphores)
    {
        chk(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &semaphore));
    }
}

auto VulkanEngine::loadShader(u64 bufferSize, u32* bufferPointer) -> Handle<ShaderTag>
{
    m_shaderModules.push_back(internalLoadShader(bufferSize, bufferPointer));
    return ShaderID(m_shaderModules.size() - 1);
}

auto VulkanEngine::reloadShader(ShaderID shader, u64 bufferSize, u32* bufferPointer) -> void
{
    u64 shaderIndex = (u64) shader;
    vkDestroyShaderModule(m_device, m_shaderModules[shaderIndex], nullptr);
    m_shaderModules[shaderIndex] = internalLoadShader(bufferSize, bufferPointer);
}

auto VulkanEngine::createPipeline(ShaderID shaderID, Blending blending) -> PipelineID
{
    VkPipelineLayout layout = createPipelineLayout();
    m_pipelines.push_back(internalCreatePipeline(layout, shaderID, blending));
    return PipelineID(m_pipelines.size() - 1);
}

auto VulkanEngine::reloadPipeline(PipelineID pipeline, ShaderID shader, Blending blending) -> void
{
    u64 pipelineIndex = (u64) pipeline;
    chk(vkDeviceWaitIdle(m_device));
    Pipeline old = m_pipelines[pipelineIndex];
    vkDestroyPipeline(m_device, old.pipeline, nullptr);
    m_pipelines[pipelineIndex] = internalCreatePipeline(old.layout, shader, blending);
}

auto VulkanEngine::internalLoadShader(u64 bufferSize, u32* bufferPointer) -> VkShaderModule
{
    VkShaderModule module;
    VkShaderModuleCreateInfo shaderModuleCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = bufferSize,
        .pCode = bufferPointer
    };
    chk(vkCreateShaderModule(m_device, &shaderModuleCI, nullptr, &module));
    return module;
}

auto VulkanEngine::createPipelineLayout() -> VkPipelineLayout
{
    VkPipelineLayout layout;
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(VkDeviceAddress)
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayoutTex,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    chk(vkCreatePipelineLayout(m_device, &pipelineLayoutCI, nullptr, &layout));

    return layout;
}

auto VulkanEngine::internalCreatePipeline(VkPipelineLayout layout, ShaderID shaderID, Blending blending) -> Pipeline
{
    Pipeline pipeline{};
    pipeline.layout = layout;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = m_shaderModules[(u64) shaderID],
            .pName = "main"
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = m_shaderModules[(u64) shaderID],
            .pName = "main"
        }
    };
    VkVertexInputBindingDescription vertexBinding{
        .binding = 0,
        .stride = 0,//sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    std::vector<VkVertexInputAttributeDescription> vertexAttributes{
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 0//offsetof(Vertex, texCoord)
        },
    };
    VkPipelineVertexInputStateCreateInfo vertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBinding,
        .vertexAttributeDescriptionCount = static_cast<u32>(vertexAttributes.size()),
        .pVertexAttributeDescriptions = vertexAttributes.data(),
    };
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };
    std::vector<VkDynamicState> dynamicStates{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates.data()
    };
    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    VkPipelineRasterizationStateCreateInfo rasterizationState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .lineWidth = 1.0f,
    };
    VkPipelineMultisampleStateCreateInfo multisampleState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
    VkPipelineDepthStencilStateCreateInfo depthStencilState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };
    VkPipelineColorBlendAttachmentState blendAttachment{
        .colorWriteMask = 0xF,
    };

    if (blending == Blending::ALPHA_BLEND)
    {
        blendAttachment = setAlphaBlendAttachment();
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
    };

    const VkFormat imageFormat{ VK_FORMAT_B8G8R8A8_UNORM};
    VkFormat depthFormat{ VK_FORMAT_D32_SFLOAT_S8_UINT };
    VkPipelineRenderingCreateInfo renderingCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &imageFormat,
        .depthAttachmentFormat = depthFormat
    };
    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCI,
        .stageCount = 2,
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = pipeline.layout
    };
    chk(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline.pipeline));

    return pipeline;
}
/*
auto VulkanEngine::loadTextureData(Texture& texture) -> void
{
    std::vector<VkDescriptorImageInfo> textureDescriptors{};
    for (u64 i = 0; i < m_textures.size(); ++i)
    {
        VkImageCreateInfo texImgCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            //.format = ktxTexture_GetVkFormat(ktxTexture),
            .format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
            .extent = {
                .width = texture.width,
                .height = texture.height,
                .depth = 1
            },
            .mipLevels = texture.mipLevels,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };
        VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
        chk(vmaCreateImage(m_allocator, &texImgCI, &texImageAllocCI, &m_textures[i].image, &m_textures[i].allocation, nullptr));
        VkImageViewCreateInfo texVewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_textures[i].image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = texImgCI.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = texture.mipLevels,
                .layerCount = 1 
            }
        };
        chk(vkCreateImageView(m_device, &texVewCI, nullptr, &m_textures[i].view));

        // Upload
        VkBuffer imgSrcBuffer{};
        VmaAllocation imgSrcAllocation{};
        VkBufferCreateInfo imgSrcBufferCI{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = texture.dataSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };
        VmaAllocationCreateInfo imgSrcAllocCI{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        VmaAllocationInfo imgSrcAllocInfo{};
        chk(vmaCreateBuffer(m_allocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo));
        memcpy(imgSrcAllocInfo.pMappedData, texture.pData.data(), texture.dataSize);
        VkFenceCreateInfo fenceOneTimeCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VkFence fenceOneTime{};
        chk(vkCreateFence(m_device, &fenceOneTimeCI, nullptr, &fenceOneTime));
        VkCommandBuffer cbOneTime{};
        VkCommandBufferAllocateInfo cbOneTimeAI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .commandBufferCount = 1
        };
        chk(vkAllocateCommandBuffers(m_device, &cbOneTimeAI, &cbOneTime));
        VkCommandBufferBeginInfo cbOneTimeBI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        chk(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI));
        VkImageMemoryBarrier2 barrierTexImage{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = m_textures[i].image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = texture.mipLevels,
                .layerCount = 1
            }
        };
        VkDependencyInfo barrierTexInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrierTexImage
        };
        vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
        std::vector<VkBufferImageCopy> copyRegions{};
        for (u32 j = 0; j < texture.mipLevels; ++j)
        {
            copyRegions.push_back({
                .bufferOffset = texture.mipOffset[j],
                .imageSubresource{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = (u32)j,
                    .layerCount = 1
                },
                .imageExtent{
                    .width = texture.width >> j,
                    .height = texture.height >> j,
                    .depth = 1
                },
            });
        }
        vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, m_textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<u32>(copyRegions.size()), copyRegions.data());
        VkImageMemoryBarrier2 barrierTexRead{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
            .image = m_textures[i].image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = texture.mipLevels,
                .layerCount = 1
            }
        };
        barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
        vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
        chk(vkEndCommandBuffer(cbOneTime));
        VkSubmitInfo oneTimeSI{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cbOneTime
        };
        chk(vkQueueSubmit(m_queue, 1, &oneTimeSI, fenceOneTime));
        chk(vkWaitForFences(m_device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
        vkDestroyFence(m_device, fenceOneTime, nullptr);
        vmaDestroyBuffer(m_allocator, imgSrcBuffer, imgSrcAllocation);

        // Sampler
        VkSamplerCreateInfo samplerCI{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = 8.0f,
            .maxLod = (f32) texture.mipLevels,
        };
        chk(vkCreateSampler(m_device, &samplerCI, nullptr, &m_textures[i].sampler));
        textureDescriptors.push_back({
            .sampler = m_textures[i].sampler,
            .imageView = m_textures[i].view,
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        });
    }

    // Descriptor (indexing)
    VkDescriptorBindingFlags descVariableFlag{
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
    };
    VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &descVariableFlag
    };
    VkDescriptorSetLayoutBinding descLayoutBindingTex{
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<u32>(m_textures.size()),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    VkDescriptorSetLayoutCreateInfo descLayoutTexCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descBindingFlags,
        .bindingCount = 1,
        .pBindings = &descLayoutBindingTex
    };
    chk(vkCreateDescriptorSetLayout(m_device, &descLayoutTexCI, nullptr, &m_descriptorSetLayoutTex));
    VkDescriptorPoolSize poolSize{
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<u32>(m_textures.size())
    };
    VkDescriptorPoolCreateInfo descPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };
    chk(vkCreateDescriptorPool(m_device, &descPoolCI, nullptr, &m_descriptorPool));
    u32 variableDescCount{ static_cast<u32>(m_textures.size()) };
    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &variableDescCount
    };
    VkDescriptorSetAllocateInfo texDescSetAlloc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variableDescCountAI,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_descriptorSetLayoutTex
    };
    chk(vkAllocateDescriptorSets(m_device, &texDescSetAlloc, &m_descriptorSetTex));
    VkWriteDescriptorSet writeDescSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_descriptorSetTex,
        .dstBinding = 0,
        .descriptorCount = static_cast<u32>(textureDescriptors.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = textureDescriptors.data()
    };
    vkUpdateDescriptorSets(m_device, 1, &writeDescSet, 0, nullptr);
}*/
