#include "core/VulkanContext.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "shared/Types.h"
#include  "shared/EngineConfig.h"

#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>

namespace {

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
    #else
    constexpr bool enableValidationLayers = true;
    #endif

    std::vector<const char*> requiredDeviceExtensions = {
        vk::KHRSwapchainExtensionName,
        vk::EXTHostQueryResetExtensionName,
    };
}

std::vector<const char*> getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity, // Diagnostic, Info, Warning, or Error
    vk::DebugUtilsMessageTypeFlagsEXT type, // General (other), validation, or performance (suboptimal vulkan)
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, // Message data: pMessage, pObjets, objectCount
    void* pUserData) {

    std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

    return vk::False;
}

void VulkanContext::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
        // | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
    );

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{ .messageSeverity = severityFlags,
                                                                          .messageType = messageTypeFlags,
                                                                          .pfnUserCallback = &debugCallback };

    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

VulkanContext::VulkanContext(GLFWwindow* window) {
    createInstance();
    setupDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createVma();
    createQueryPools();
    createContextCommandPool();
}

VulkanContext::~VulkanContext() {
    vmaDestroyAllocator(allocator);
}

void VulkanContext::createInstance() {

    // Create app info

    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    // Check if required validation layers are supported by Vulkan

    std::vector<char const*> requiredLayers;
    if (enableValidationLayers)
    {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    auto layerProperties = context.enumerateInstanceLayerProperties();
    auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
        [&layerProperties](auto const& requiredLayer) {
            return std::ranges::none_of(layerProperties,
                [requiredLayer](auto const& layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
        });
    if (unsupportedLayerIt != requiredLayers.end())
    {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }

    // Check if the required GLFW extensions are supported by Vulkan implementation

    std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();

    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (auto& extension : requiredExtensions)
    {
        if (std::ranges::none_of(extensionProperties,
            [glfwExtension = extension](auto const& extensionProperty)
            { return strcmp(extensionProperty.extensionName, glfwExtension) == 0; }))
        {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(extension));
        }
    }

    // Temp debugger for instance creation

    // Inside createInstance()
    vk::DebugUtilsMessengerCreateInfoEXT debugInfo{
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &debugCallback
    };

    // Create instance info

    vk::InstanceCreateInfo createInfo{
        .pNext = enableValidationLayers ? debugInfo : nullptr,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    instance = vk::raii::Instance(context, createInfo);
}

void VulkanContext::createSurface(GLFWwindow* window) {
    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
        throw std::runtime_error("Failed to create window surface!");
    }

    surface = vk::raii::SurfaceKHR(instance, _surface);
}

void VulkanContext::pickPhysicalDevice() { // Just checks if physical device is compatible with vk 1.3 and extensions specified in deviceExtensions

    std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

    const auto devIter = std::ranges::find_if(devices,
        [&](auto const& device) {
            auto queueFamilies = device.getQueueFamilyProperties();
            bool isSuitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
            const auto qfpIter = std::ranges::find_if(queueFamilies,
                [](vk::QueueFamilyProperties const& qfp)
                {
                    return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
                });
            isSuitable = isSuitable && (qfpIter != queueFamilies.end());
            auto extensions = device.enumerateDeviceExtensionProperties();
            bool found = true;
            for (auto const& extension : requiredDeviceExtensions) {
                auto extensionIter = std::ranges::find_if(extensions, [extension](auto const& ext) {return strcmp(ext.extensionName, extension) == 0; });
                found = found && extensionIter != extensions.end();
            }
            isSuitable = isSuitable && found;
            if (isSuitable) {
                physicalDevice = device;
            }
            return isSuitable;
        });
    if (devIter == devices.end()) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanContext::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // get first index that supports graphics
    auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const& qfp)
        { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });

    graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    // determine a queueFamilyIndex that supports present
    // first check if the graphicsIndex is good enough
    auto presentIndex = physicalDevice.getSurfaceSupportKHR(graphicsIndex, *surface)
        ? graphicsIndex
        : static_cast<uint32_t>(queueFamilyProperties.size());
    if (presentIndex == queueFamilyProperties.size())
    {
        // the graphicsIndex doesn't support present -> look for another family index that supports both
        // graphics and present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
            {
                graphicsIndex = static_cast<uint32_t>(i);
                presentIndex = graphicsIndex;
                break;
            }
        }
        if (presentIndex == queueFamilyProperties.size())
        {
            // there's nothing like a single family index that supports both graphics and present -> look for another
            // family index that supports present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
                {
                    presentIndex = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }
    if ((graphicsIndex == queueFamilyProperties.size()) || (presentIndex == queueFamilyProperties.size()))
    {
        throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
    }

    // query for Vulkan 1.3 features
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2, 
        vk::PhysicalDeviceVulkan11Features, 
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features, 
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},                              // vk::PhysicalDeviceFeatures2
        { .shaderDrawParameters = true }, // Vulkan 1.1 features
        { .hostQueryReset = true },      // Vulkan 1.2 features
        {
            .synchronization2 = true,
            .dynamicRendering = true,
        },                               // Vulkan 1.3 features
        { .extendedDynamicState = true}, // Dynamic State Extension features
    };

    // create a Device
    float                     queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
    vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                               .queueCreateInfoCount = 1,
                                               .pQueueCreateInfos = &deviceQueueCreateInfo,
                                               .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
                                               .ppEnabledExtensionNames = requiredDeviceExtensions.data() };

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
    presentQueue = vk::raii::Queue(device, presentIndex, 0);
}

void VulkanContext::createVma() {
    VmaVulkanFunctions vulkanFunctions = {
        .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = &vkGetDeviceProcAddr
    };

    VmaAllocatorCreateInfo allocatorCreateInfo = {};

    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_4;
    allocatorCreateInfo.physicalDevice = *physicalDevice;
    allocatorCreateInfo.device = *device;
    allocatorCreateInfo.instance = *instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    vmaCreateAllocator(&allocatorCreateInfo, &allocator);
}

void VulkanContext::createContextCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsIndex
    };

    contextCommandPool = vk::raii::CommandPool(device, poolInfo);
}

AllocatedBuffer VulkanContext::createVmaBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags allocationFlags, VmaMemoryUsage allocationUsage) {
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };

    bufferInfo.size = size;
    bufferInfo.usage = bufferUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo allocInfo = {
        .flags = allocationFlags,
        .usage = allocationUsage
    };

    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;

    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &info);

    return { buffer, allocation, info };
}

void VulkanContext::copyBuffer(AllocatedBuffer src, AllocatedBuffer dst, vk::DeviceSize size) {
    vk::CommandBufferAllocateInfo allocInfo{ .commandPool = contextCommandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };
    vk::raii::CommandBuffer commandCopyBuffer = std::move(device.allocateCommandBuffers(allocInfo).front());

    commandCopyBuffer.begin(vk::CommandBufferBeginInfo{ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    commandCopyBuffer.copyBuffer(src.buffer, dst.buffer, vk::BufferCopy(0, 0, size));
    commandCopyBuffer.end();

    graphicsQueue.submit(vk::SubmitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer }, nullptr);
    graphicsQueue.waitIdle();
}

AllocatedImage VulkanContext::createVmaImage(vk::ImageCreateInfo info, VmaAllocationCreateInfo allocCreateInfo) {
    VkImage image;
    VmaAllocation alloc;

    VkImageCreateInfo c_info = static_cast<VkImageCreateInfo>(info);

    if (vmaCreateImage(allocator, &c_info, &allocCreateInfo, &image, &alloc, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate VMA image!");
    }

    return { vk::Image(image), alloc };
}

void VulkanContext::destroyVmaImage(vk::Image& image, VmaAllocation& allocation) {
    if (image && allocation) {
        vmaDestroyImage(allocator, static_cast<VkImage>(image), allocation);

        // So we don't accidentally destroy twice
        image = nullptr;
        allocation = nullptr;
    }
}

void VulkanContext::createQueryPools() {
    vk::PhysicalDeviceLimits device_limits = physicalDevice.getProperties().limits;
    if (device_limits.timestampPeriod == 0)
    {
        throw std::runtime_error{ "The selected device does not support timestamp queries!" };
    }

    if (!device_limits.timestampComputeAndGraphics)
    {
        // Check if the graphics queue used in this sample supports time stamps
        vk::QueueFamilyProperties graphics_queue_family_properties = physicalDevice.getQueueFamilyProperties()[graphicsIndex];
        if (graphics_queue_family_properties.timestampValidBits == 0)
        {
            throw std::runtime_error{ "The selected graphics queue family does not support timestamp queries!" };
        }
    }

    timestamps.resize(EngineConfig::NUM_TIMESTAMPS);

    vk::QueryPoolCreateInfo timestampPoolInfo;
    timestampPoolInfo.sType = vk::StructureType::eQueryPoolCreateInfo;
    timestampPoolInfo.queryType = vk::QueryType::eTimestamp;
    timestampPoolInfo.queryCount = static_cast<uint32_t>(timestamps.size());

    timestampQueryPool = device.createQueryPool(timestampPoolInfo);
}

void VulkanContext::resetQueryPool(uint32_t frameIndex) {
    uint32_t startIndex = frameIndex * EngineConfig::TIMESTAMPS_PER_FRAME;
    (*device).resetQueryPool(*timestampQueryPool, startIndex, EngineConfig::TIMESTAMPS_PER_FRAME);
}

double VulkanContext::getRenderPassTime(uint32_t frameIndex) {
    uint32_t startIndex = frameIndex * EngineConfig::TIMESTAMPS_PER_FRAME;

    vk::Result result = (*device).getQueryPoolResults(
        timestampQueryPool,
        startIndex,
        EngineConfig::TIMESTAMPS_PER_FRAME,
        EngineConfig::TIMESTAMPS_PER_FRAME * sizeof(uint64_t),
        &timestamps[startIndex],
        sizeof(uint64_t),
        vk::QueryResultFlagBits::e64
    );

    assert(result == vk::Result::eSuccess);

    vk::PhysicalDeviceLimits const& deviceLimits = physicalDevice.getProperties().limits;
    float deltaInMS = float(timestamps[startIndex+1] - timestamps[startIndex]) * deviceLimits.timestampPeriod / 1000000.0f;

    return deltaInMS;
}