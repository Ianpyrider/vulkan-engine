#pragma once

#include "shared/VulkanInclude.h"

#include "glm/glm.hpp"

struct Vertex
{

    glm::vec3 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions();
};

