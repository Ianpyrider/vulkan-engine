#include "renderer\Particle.h"

std::vector<vk::VertexInputAttributeDescription> Particle::getAttributeDescriptions() {
    std::vector descriptions{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Particle, position))
    };

    return descriptions;
}

vk::VertexInputBindingDescription Particle::getBindingDescription() {
    return { 0,  sizeof(Particle), vk::VertexInputRate::eVertex };
}