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
	AllocatedImage& getTextureImage() { return textureImage; }
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

	AllocatedImage textureImage;
	vk::raii::ImageView textureImageView = nullptr;
	vk::raii::Sampler textureSampler = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;
	vk::Format textureFormat = vk::Format::eR8G8B8A8Unorm;

	PushConstantBlock pbrPushConstants;

	AllocatedBuffer createBuffer(const void* data, size_t bufferSize, VkBufferUsageFlags usage);
	AllocatedImage createImage(unsigned char* pixels, int texWidth, int texHeight, int texChannels, vk::DeviceSize imageSize);
	AllocatedImage createImageFromKTXFile(std::string filename);
	void createImageView();
	void createTextureSampler();

	void createMeshDescriptorSet(vk::raii::DescriptorPool& descriptorPool);

	void loadFromGltf(const std::string& filepath);

	void setMaterial(const tinygltf::Material& gltfMaterial);
};
