#include "renderer/Vertex.h"

std::vector<vk::VertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
    {
        std::vector descriptions {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
        };

        return descriptions;
    }
}

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
}