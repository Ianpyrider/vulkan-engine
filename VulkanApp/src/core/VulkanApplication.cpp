#include "shared/VulkanInclude.h"

#include "core/VulkanApplication.h"

#include "core/VulkanContext.h"
#include "core/SwapChainManager.h"
#include "renderer/GraphicsPipeline.h"
#include "renderer/ComputePipeline.h"
#include "renderer/Renderer.h"

#include <memory>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

VulkanApplication::VulkanApplication() {
    initWindow();

    vkContext = std::make_unique<VulkanContext>(window);

    vkSwapChainManager = std::make_unique<SwapChainManager>(*vkContext, window);

    vkGraphicsPipeline = std::make_unique<GraphicsPipeline>(*vkContext, *vkSwapChainManager);
    vkComputePipeline = std::make_unique<ComputePipeline>(*vkContext, *vkSwapChainManager);

    vkRenderer = std::make_unique<Renderer>(*vkContext, *vkSwapChainManager, *vkGraphicsPipeline, *vkComputePipeline);
}

VulkanApplication::~VulkanApplication() {
    if (vkContext != nullptr && vkContext->getDevice() != nullptr) {
        vkContext->getDevice().waitIdle();
    }

    // Explicitly destroy in reverse order
    vkRenderer.reset();           
    vkComputePipeline.reset();    
    vkGraphicsPipeline.reset();   

    vkSwapChainManager->cleanupSwapChain();
    vkSwapChainManager.reset();   

    vkContext.reset();

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

