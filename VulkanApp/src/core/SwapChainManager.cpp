#include "core/SwapChainManager.h"

#include <algorithm>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "shared/EngineConfig.h"
#include "core/VulkanContext.h"

SwapChainManager::SwapChainManager(VulkanContext& context, GLFWwindow* window) 
    : context(context), window(window) 
{
    createSwapChain();
    createImageViews();
    createDepthResources();
}

SwapChainManager::~SwapChainManager() {
    vmaDestroyImage(context.getVmaAllocator(), depthImage.image, depthImage.allocation);
}

void SwapChainManager::cleanupSwapChain() {
    swapChainImageViews.clear();
    swapChain = nullptr;
}

// Create swap chain functions

void SwapChainManager::createSwapChain() {
    auto& physicalDevice = context.getPhysicalDevice();
    auto& surface = context.getSurface();
    auto& device = context.getDevice();

    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface));
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) ? surfaceCapabilities.maxImageCount : minImageCount;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = *surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
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

vk::SurfaceFormatKHR SwapChainManager::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eR8G8B8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0]; //Settle for whatever's provided, optimization via scoring possible
}

vk::PresentModeKHR SwapChainManager::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    if (!EngineConfig::ENABLE_MAILBOX_PRESENT) {
        return vk::PresentModeKHR::eFifo;
    }
    
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChainManager::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
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

void SwapChainManager::recreateSwapChain() {
    auto& device = context.getDevice();

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
    createDepthResources();
}

// Image View Functions

void SwapChainManager::createImageViews() {
    auto& device = context.getDevice();

    swapChainImageViews.clear();

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat.format,
        .subresourceRange = {
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        }
    };

    for (auto image : swapChainImages) {
        imageViewCreateInfo.image = image; // Identical infos, different images. I guess we could want different settings per image of those above, but idk when that would happen
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
}

void SwapChainManager::createDepthResources() {
    depthFormat = findSupportedFormat(
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );

    vk::ImageCreateInfo createImageInfo{
        .imageType = vk::ImageType::e2D,
        .format = depthFormat,
        .extent = {swapChainExtent.width, swapChainExtent.height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
    };

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocCreateInfo.priority = 1.0f;

    depthImage = context.createVmaImage(createImageInfo, allocCreateInfo);

    vk::ImageViewCreateInfo viewInfo{
        .image = depthImage.image,
        .viewType = vk::ImageViewType::e2D,
        .format = depthFormat,
        .subresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
    };

    depthImageView = vk::raii::ImageView(context.getDevice(), viewInfo);
}

vk::Format SwapChainManager::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (const auto format : candidates) {
        vk::FormatProperties props = context.getPhysicalDevice().getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}