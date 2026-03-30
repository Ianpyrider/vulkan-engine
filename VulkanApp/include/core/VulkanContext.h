#pragma once

#include "shared/VulkanInclude.h"

#include "shared/Types.h"
#include "vma/vk_mem_alloc.h"

struct GLFWwindow;

class VulkanContext {
public:
    VulkanContext(GLFWwindow* window);
    ~VulkanContext();

    uint32_t getGraphicsIndex() const { return graphicsIndex; }
    vk::raii::PhysicalDevice& getPhysicalDevice() { return physicalDevice; }
    vk::raii::SurfaceKHR& getSurface() { return surface; }
    vk::raii::Device& getDevice() { return device; }
    vk::raii::Queue& getGraphicsQueue() { return graphicsQueue; }
    vk::raii::Queue& getPresentQueue() { return presentQueue; }
    VmaAllocator& getVmaAllocator() { return allocator; }
    vk::raii::QueryPool& getTimestampQueryPool() { return timestampQueryPool; }
    AllocatedBuffer createVmaBuffer(VkDeviceSize size, VkBufferUsageFlags bufferFlags, VmaAllocationCreateFlags allocationFlags, VmaMemoryUsage usage);
    void copyBuffer(AllocatedBuffer src, AllocatedBuffer dst, vk::DeviceSize size);
    AllocatedImage createVmaImage(vk::ImageCreateInfo info, VmaAllocationCreateInfo allocCreateInfo);
    void destroyVmaImage(vk::Image& image, VmaAllocation& allocation);
    void resetQueryPool(uint32_t frameIndex);
    double getRenderPassTime(uint32_t frameIndex);
private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createVma();
    void createQueryPools();
    void createContextCommandPool();

    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    uint32_t graphicsIndex = ~0; // Index of the graphics queue family
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr; // There are some cases where graphics queue doesn't support present, which we check for below. Note that default is same as graphics though. 
    vk::raii::SurfaceKHR surface = nullptr;
    VmaAllocator allocator = nullptr;
    vk::raii::QueryPool timestampQueryPool = nullptr;
    std::vector<uint64_t> timestamps = {};
    vk::raii::CommandPool contextCommandPool = nullptr;
};