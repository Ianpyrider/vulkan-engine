#pragma once

#include "VulkanInclude.h"

struct GLFWwindow;

class VulkanContext {
public:
    VulkanContext(GLFWwindow* window);
    ~VulkanContext() = default; // Yay for RAII!

    uint32_t getGraphicsIndex() const { return graphicsIndex; }
    vk::raii::PhysicalDevice& getPhysicalDevice() { return physicalDevice; }
    vk::raii::SurfaceKHR& getSurface() { return surface; }
    vk::raii::Device& getDevice() { return device; }
    vk::raii::Queue& getGraphicsQueue() { return graphicsQueue; }
    vk::raii::Queue& getPresentQueue() { return presentQueue; }
private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();

    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    uint32_t graphicsIndex = ~0;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr; // There are some cases where graphics queue doesn't support present, which we check for below. Note that default is same as graphics though. 
    vk::raii::SurfaceKHR surface = nullptr;
};