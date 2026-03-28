#include "shared/VulkanUtils.h"

#include "core/VulkanContext.h"

namespace VulkanUtils {
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(vk::raii::Device& device , const std::vector<char>& code) {
        vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
        vk::raii::ShaderModule shaderModule{ device, createInfo };
        return shaderModule;
    }
}