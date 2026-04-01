#pragma once

#include "shared/VulkanInclude.h"

#include "shared/Types.h"
#include "Vertex.h"

#include "glm/glm.hpp"

class VulkanContext;

class Mesh
{
public:
	Mesh(const std::string& filepath, VulkanContext& context);
	~Mesh();

	AllocatedBuffer& getVertexBuffer() { return vertexBuffer; }
	AllocatedBuffer& getIndexBuffer() { return indexBuffer; }
	uint32_t getIndexCount() { return indexCount; };
private:
	VulkanContext& context;

	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	uint32_t indexCount;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	AllocatedBuffer createBuffer(const void* data, size_t bufferSize, VkBufferUsageFlags usage);
	void loadFromObj(const std::string& filepath);
};
