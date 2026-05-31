#pragma once

#include "shared/VulkanInclude.h"
#include "vma/vk_mem_alloc.h"
#include <glm/glm.hpp>

struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VmaAllocationInfo info = {};
};

struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
};

struct UBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Snow_UBO {
    glm::float32 deltaTime;
    glm::float32 totalTime;
};

struct PBR_UBO {
    glm::mat4 model; //64
    glm::mat4 view; //64
    glm::mat4 proj; //64
    glm::vec4 lightPositions[4]; //
    glm::vec4 lightColors[4]; //
    glm::vec4 camPos;
    glm::float32 exposure;
    glm::float32 gamma;
    glm::float32 prefilteredCubeMipLevels;
    glm::float32 scaleIBLAmbient;
};

struct PushConstantBlock {
    glm::vec4 baseColorFactor;
    glm::float32 metallicFactor;
    glm::float32 roughnessFactor;
    glm::int32 baseColorTextureSet;
    glm::int32 physicalDescriptorTextureSet;
    glm::int32 normalTextureSet;
    glm::int32 occlusionTextureSet;
    glm::int32 emissiveTextureSet;
    glm::float32 alphaMask;
    glm::float32 alphaMaskCutoff;
};