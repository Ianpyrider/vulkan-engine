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
	const PushConstantBlock& getPbrPushConstants() const { return pbrPushConstants; }
private:
	VulkanContext& context;

	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	uint32_t indexCount;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	PushConstantBlock pbrPushConstants;

	AllocatedBuffer createBuffer(const void* data, size_t bufferSize, VkBufferUsageFlags usage);
	void loadFromObj(const std::string& filepath);

	void setDefaultMaterial() {
		pbrPushConstants.baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		pbrPushConstants.metallicFactor = 1.0f;
		pbrPushConstants.roughnessFactor = 0.3f;

		pbrPushConstants.baseColorTextureSet = -1;
		pbrPushConstants.physicalDescriptorTextureSet = -1; 
		pbrPushConstants.normalTextureSet = -1;
		pbrPushConstants.occlusionTextureSet = -1;
		pbrPushConstants.emissiveTextureSet = -1;

		pbrPushConstants.alphaMask = 0.0f;
		pbrPushConstants.alphaMaskCutoff = 0.5f;
	}
};
