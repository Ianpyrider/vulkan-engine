#pragma once

#include "VulkanInclude.h"
#include "vma/vk_mem_alloc.h"

struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo info = {};
};