#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <cstdint> // Necessary for uint32_t
#include <algorithm> // Necessary for std::clamp

#include <fstream>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    uint32_t graphicsIndex = ~0;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr; // There are some cases where graphics queue doesn't support present, which we check for below. Note that default is same as graphics though. 
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::SwapchainKHR swapChain = nullptr;

    std::vector<vk::Image> swapChainImages;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::raii::ImageView> swapChainImageViews;

    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;

    vk::raii::CommandPool commandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;

    std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;

    bool framebufferResized = false;

    uint32_t frameIndex = 0;

    std::vector<const char*> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName
    };

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();

        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        device.waitIdle();
    }

    void cleanupSwapChain() {
        swapChainImageViews.clear();
        swapChain = nullptr;
    }

    void cleanup() {
        cleanupSwapChain();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    // Instance functions

    void createInstance() {

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

        auto requiredExtensions = getRequiredInstanceExtensions();

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

        // Create instance info

        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data(),
        };

        instance = vk::raii::Instance(context, createInfo);
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

    // Debug functions

    void setupDebugMessenger() {
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

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity, // Diagnostic, Info, Warning, or Error
        vk::DebugUtilsMessageTypeFlagsEXT type, // General (other), validation, or performance (suboptimal vulkan)
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, // Message data: pMessage, pObjets, objectCount
        void* pUserData) {

        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

        return vk::False;
    }

    // Physical Device Functions

    void pickPhysicalDevice() { // Just checks if physical device is compatible with vk 1.3 and extensions specified in deviceExtensions

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
                for (auto const& extension : requiredDeviceExtension) {
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

    // Logical Device Functions

    void createLogicalDevice() {
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
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},         // vk::PhysicalDeviceFeatures2
            {.shaderDrawParameters = true }, // Features from vulkan 1.1
            { 
                .synchronization2 = true,
                .dynamicRendering = true
            },           // vk::PhysicalDeviceVulkan13Features
            { .extendedDynamicState = true}        // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        };

        // create a Device
        float                     queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
        vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                                   .queueCreateInfoCount = 1,
                                                   .pQueueCreateInfos = &deviceQueueCreateInfo,
                                                   .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
                                                   .ppEnabledExtensionNames = requiredDeviceExtension.data() };

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);
        graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
        presentQueue = vk::raii::Queue(device, presentIndex, 0);
    }

    // Window Surface Functions

    void createSurface() {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0) {
            throw std::runtime_error("Failed to create window surface!");
        }

        surface = vk::raii::SurfaceKHR(instance, _surface);
    }

    // Swap Chain Funcitons

    void createSwapChain() {
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
        swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface));
        swapChainExtent = chooseSwapExtent(surfaceCapabilities);
        auto minImageCount = max(3u, surfaceCapabilities.minImageCount);
        minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) ? surfaceCapabilities.maxImageCount : minImageCount;

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

        if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{
            .flags = vk::SwapchainCreateFlagsKHR(),
            .surface = *surface,
            .minImageCount = minImageCount,
            .imageFormat = swapChainSurfaceFormat.format,
            .imageColorSpace = swapChainSurfaceFormat.colorSpace,
            .imageExtent = swapChainExtent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform = surfaceCapabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(*surface)),
            .clipped = true,
            .oldSwapchain = nullptr
        };

        swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
        swapChainImages = swapChain.getImages();
    }

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }

        return availableFormats[0]; //Settle for whatever's provided, optimization via scoring possible
    }

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != 0xFFFFFFFF)
        {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
    }

    void recreateSwapChain() {

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device.waitIdle();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
    }

    // Image View Functions

    void createImageViews() {
        swapChainImageViews.clear();

        vk::ImageViewCreateInfo imageViewCreateInfo{ 
            .viewType = vk::ImageViewType::e2D, 
            .format = swapChainSurfaceFormat.format, 
            .subresourceRange = { 
                vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
            } 
        };

        for (auto image : swapChainImages) {
            imageViewCreateInfo.image = image; // Identical infos, different images. I guess we could want different settings per image of those above.
            swapChainImageViews.emplace_back(device, imageViewCreateInfo);
        }
    }

    // Graphics Pipeline Creation Functions

    void createGraphicsPipeline() {

        // Create shaderModule, which takes in vertex and frag shaders here. Basically specify and program the pipeline steps you want to

        vk::raii::ShaderModule shaderModule = createShaderModule(readFile("slang.spv"));

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shaderModule,
            .pName = "vertMain"
        };

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shaderModule,
            .pName = "fragMain"
        };

        vk::PipelineShaderStageCreateInfo shaderStages[] = {
            vertShaderStageInfo,
            fragShaderStageInfo
        };

        // Set dynamic states

        std::vector dynamicStates = { // Optionally specify settings to be defined at draw time, MUST BE SPECIFIED THEN IF YOU DO THIS
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamicState{
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        // Specify other pipeline settings
       
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo; // Specify format of vertex info, currently hardcoded in shader so this is empty

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
            .topology = vk::PrimitiveTopology::eTriangleList
        };

        //vk::Viewport{ 0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f };
        //vk::Rect2D{ vk::Offset2D{ 0, 0 }, swapChainExtent };

        // Notice these are what we marked as dynamic state earlier, now specified in the viewport state
        vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };

        vk::PipelineRasterizationStateCreateInfo rasterizer{
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
            .depthBiasSlopeFactor = 1.0f,
            .lineWidth = 1.0f
        };

        vk::PipelineMultisampleStateCreateInfo multisampling{ 
            .rasterizationSamples = vk::SampleCountFlagBits::e1, 
            .sampleShadingEnable = vk::False
        };

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
        };

        vk::PipelineColorBlendStateCreateInfo colorBlending{ 
            .logicOpEnable = vk::False, 
            .logicOp = vk::LogicOp::eCopy, 
            .attachmentCount = 1, 
            .pAttachments = &colorBlendAttachment
        };

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
            .setLayoutCount = 0, 
            .pushConstantRangeCount = 0
        };

        pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{ .colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format };

        vk::GraphicsPipelineCreateInfo pipelineInfo{ 
            .pNext = &pipelineRenderingCreateInfo,
            .stageCount = 2, 
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo, 
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,   
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState, 
            .layout = pipelineLayout, 
            .renderPass = nullptr
        };

        graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        file.close();

        return buffer;
    }

    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const {
        vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
        vk::raii::ShaderModule shaderModule{ device, createInfo };
        return shaderModule;
    }

    // Command Pool functions

    void createCommandPool() {
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = graphicsIndex
        };

        commandPool = vk::raii::CommandPool(device, poolInfo);
    }

    void createCommandBuffers() {
        commandBuffers.clear();

        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };

        commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    }

    void recordCommandBuffer(uint32_t imageIndex) {
        auto &commandBuffer = commandBuffers[frameIndex];

        commandBuffer.begin({});

        // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
        transition_image_layout(
            imageIndex,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},                                                         // srcAccessMask (no need to wait for previous operations)
            vk::AccessFlagBits2::eColorAttachmentWrite,                 // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,         // srcStage
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,          // dstStage
            commandBuffer
        );

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f); 
        vk::RenderingAttachmentInfo attachmentInfo = { 
            .imageView = swapChainImageViews[imageIndex], 
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal, 
            .loadOp = vk::AttachmentLoadOp::eClear, 
            .storeOp = vk::AttachmentStoreOp::eStore, 
            .clearValue = clearColor 
        };

        vk::RenderingInfo renderingInfo = {
            .renderArea = {.offset = { 0, 0 }, .extent = swapChainExtent },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo
        };

        commandBuffer.beginRendering(renderingInfo); // WAHOOOOOOOOOOOOOOOOOOOOOOOOOOOO

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

        // Wow, we set the dynamic settings we specified earlier!
        commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
        commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));

        commandBuffer.draw(3, 1, 0, 0);

        commandBuffer.endRendering();

        // After rendering, transition the swapchain image to PRESENT_SRC
        transition_image_layout(
            imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
            {},                                                     // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
            commandBuffer
        );

        commandBuffer.end();
    }

    void transition_image_layout(
        uint32_t imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::AccessFlags2 srcAccessMask,
        vk::AccessFlags2 dstAccessMask,
        vk::PipelineStageFlags2 srcStageMask,
        vk::PipelineStageFlags2 dstStageMask,
        vk::raii::CommandBuffer &curCommandBuffer
    ) {
        vk::ImageMemoryBarrier2 barrier = {
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = swapChainImages[imageIndex],
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };
        curCommandBuffer.pipelineBarrier2(dependencyInfo);
    }

    // Sync functions

    void createSyncObjects() {
        // Semaphore: Order execution on the GPU
        // Fence: Block CPU (host) until signaled
        // Knowledge check: Why the thee sync objects specified in the tutorial?
        // Knowledge check: Why are there two loops here?

        assert(presentCompleteSemaphores.empty() && renderFinishedSemaphores.empty() && inFlightFences.empty());
        
        for (size_t i = 0; i<MAX_FRAMES_IN_FLIGHT; i++) {
            presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo())); // Notice device is specified
            inFlightFences.emplace_back(vk::raii::Fence(device, { .flags = vk::FenceCreateFlagBits::eSignaled }));
        }

        for (size_t i = 0; i<swapChainImages.size(); i++) {
            renderFinishedSemaphores.emplace_back(vk::raii::Semaphore(device, vk::SemaphoreCreateInfo()));
        }
    }

    // DRAW FRAME!!!!

    void drawFrame() {
        // Making sure we're not fenced here means we wait until we get a signal from graphicsQueue.submit
        auto fenceResult = device.waitForFences(*inFlightFences[frameIndex], vk::True, UINT64_MAX);

        if (fenceResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to wait for fence!");
        }

        auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[frameIndex], nullptr);

        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            recreateSwapChain();
            return;
        }
        else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // Only reset the fence if we are submitting work
        device.resetFences(*inFlightFences[frameIndex]);

        commandBuffers[frameIndex].reset();
        recordCommandBuffer(imageIndex);

        vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        const vk::SubmitInfo submitInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*presentCompleteSemaphores[frameIndex],
            .pWaitDstStageMask = &waitDestinationStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffers[frameIndex],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex] // Knowledge check: Why is this imageIndex and not frameIndex?
        };

        graphicsQueue.submit(submitInfo, *inFlightFences[frameIndex]);

        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1,
            .pSwapchains = &*swapChain,
            .pImageIndices = &imageIndex
        };

        result = presentQueue.presentKHR(presentInfoKHR);

        if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || framebufferResized)
        {
            framebufferResized = false;
            recreateSwapChain();
        }
        else
        {
            // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
            assert(result == vk::Result::eSuccess);
        }


        frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }
};

int main()
{
    try
    {
        HelloTriangleApplication app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}