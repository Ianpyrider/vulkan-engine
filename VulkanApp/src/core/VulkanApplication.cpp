#include "shared/VulkanInclude.h"

#include "core/VulkanApplication.h"

#include "shared/EngineConfig.h"
#include "core/VulkanContext.h"
#include "core/SwapChainManager.h"
#include "renderer/GraphicsPipeline.h"
#include "renderer/ImageComputePipeline.h"
#include "renderer/Renderer.h"

#include <memory>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

VulkanApplication::VulkanApplication() {
    initWindow();

    vkContext = std::make_unique<VulkanContext>(window);

    vkSwapChainManager = std::make_unique<SwapChainManager>(*vkContext, window);

    vkGraphicsPipeline = std::make_unique<GraphicsPipeline>(*vkContext, *vkSwapChainManager);
    vkImageComputePipeline = std::make_unique<ImageComputePipeline>(*vkContext, *vkSwapChainManager);

    vkRenderer = std::make_unique<Renderer>(*vkContext, *vkSwapChainManager, *vkGraphicsPipeline, *vkImageComputePipeline);
}

VulkanApplication::~VulkanApplication() {
    if (vkContext != nullptr && vkContext->getDevice() != nullptr) {
        vkContext->getDevice().waitIdle();
    }

    vkSwapChainManager->cleanupSwapChain(); 

    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanApplication::run() {
    mainLoop();
}

void VulkanApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        vkRenderer->drawFrame();
    }

    vkContext->getDevice().waitIdle();
}

void VulkanApplication::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanApplication::notifyFramebufferResized() {
    vkRenderer->setFrameBufferResized();
}

void VulkanApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
    app->notifyFramebufferResized();
}

