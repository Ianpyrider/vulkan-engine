#pragma once

#include "shared/VulkanInclude.h"

namespace VulkanUtils
{
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(vk::raii::Device& device, const std::vector<char>& code);
};

