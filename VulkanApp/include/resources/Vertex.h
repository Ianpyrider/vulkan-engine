#pragma once

#include "shared/VulkanInclude.h"

#include "glm/glm.hpp"

struct Vertex
{

    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();
};

