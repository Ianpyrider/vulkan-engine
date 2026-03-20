#include "vulkanUtils.h"

#include "VulkanContext.h"

namespace vulkanUtils {
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(vk::raii::Device& device , const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
        vk::raii::ShaderModule shaderModule{ device, createInfo };
        return shaderModule;
    }
}