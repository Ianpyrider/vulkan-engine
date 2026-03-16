#include "VulkanInclude.h"

#include "VulkanApplication.h"

#include "VulkanContext.h"
#include "SwapChainManager.h"
#include "GraphicsPipeline.h"
#include "Renderer.h"

#include <memory>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

VulkanApplication::VulkanApplication() {
    initWindow();

    vkContext = std::make_unique<VulkanContext>(window);
    vkSwapChainManager = std::make_unique<SwapChainManager>(*vkContext, window);
    vkGraphicsPipeline = std::make_unique<GraphicsPipeline>(*vkContext, *vkSwapChainManager);
    vkRenderer = std::make_unique<Renderer>(*vkContext, *vkSwapChainManager, *vkGraphicsPipeline);
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

    window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
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

