#pragma once

#include "VulkanInclude.h"

namespace vulkanUtils
{
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(vk::raii::Device& device, const std::vector<char>& code);
};

