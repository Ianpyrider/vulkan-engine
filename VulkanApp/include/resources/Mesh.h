#pragma once

#include "shared/VulkanInclude.h"

#include "shared/Types.h"
#include "Vertex.h"

#include "glm/glm.hpp"
#include <tiny_gltf/tiny_gltf.h>

class VulkanContext;
class SwapChainManager;
class GraphicsPipeline;

class Mesh
{
public:
	Mesh(
		const std::string& filepath,
		VulkanContext& context,
		SwapChainManager& swapChainManager,
		GraphicsPipeline& graphicsPipeline,
		vk::raii::DescriptorPool& descriptorPool
	);
	~Mesh();

	AllocatedBuffer& getVertexBuffer() { return vertexBuffer; }
	AllocatedBuffer& getIndexBuffer() { return indexBuffer; }
	std::vector<vk::raii::DescriptorSet>& getDescriptorSets() { return descriptorSets; }

	uint32_t getIndexCount() { return indices.size(); };
	const PushConstantBlock& getPbrPushConstants() const { return pbrPushConstants; }
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;
	GraphicsPipeline& graphicsPipeline;

	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	AllocatedImage irradianceTexture;
	vk::raii::ImageView irradianceImageView = nullptr;
	vk::raii::Sampler irradianceSampler = nullptr;

	AllocatedImage radianceTexture;
	vk::raii::ImageView radianceImageView = nullptr;
	vk::raii::Sampler radianceSampler = nullptr;

	AllocatedImage brdfTexture;
	vk::raii::ImageView brdfImageView = nullptr;
	vk::raii::Sampler brdfSampler = nullptr;

	std::vector<vk::raii::DescriptorSet> descriptorSets;

	PushConstantBlock pbrPushConstants;

	AllocatedBuffer createBuffer(const void* data, size_t bufferSize, VkBufferUsageFlags usage);
	AllocatedImage createCubeImageFromKTX(std::string filename);
	AllocatedImage create2DImageFromKTX(std::string filename);

	void createImageViews();
	void createTextureSamplers();

	void createMeshDescriptorSet(vk::raii::DescriptorPool& descriptorPool);

	void loadFromGltf(const std::string& filepath);

	void setMaterial(const tinygltf::Material& gltfMaterial);
};
