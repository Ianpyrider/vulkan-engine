#pragma once

#include "shared/VulkanInclude.h"

#include "glm/glm.hpp"

struct Particle { // Should probably make an abstract class parent for this and vertex
	glm::vec4 position;
	glm::vec4 velocity;

	static vk::VertexInputBindingDescription getBindingDescription();
	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();
};
