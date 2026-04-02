#pragma once

#include "shared/VulkanInclude.h"

#include "shared/Types.h"

class VulkanContext;
class SwapChainManager;

class ParticleComputePipeline
{
public:
	ParticleComputePipeline(VulkanContext& context, SwapChainManager& swapChainManager);
	~ParticleComputePipeline() = default;

	vk::raii::Pipeline& getComputePipeline() { return pipeline; }
	vk::raii::PipelineLayout& getPipelineLayout() { return pipelineLayout; }
	vk::raii::DescriptorSets& getDescriptorSets() { return computeDescriptorSets; };

	AllocatedBuffer& getParticleBuffer() { return particleBuffer; }
	AllocatedBuffer& getUniformBuffer() { return uniformBuffer; };

	void updateParticleUBO(float totalTime, float deltaTime);
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;

	vk::raii::Pipeline pipeline = nullptr;
	vk::raii::PipelineLayout pipelineLayout = nullptr;

	vk::raii::DescriptorSets computeDescriptorSets{ nullptr };
	vk::raii::DescriptorSetLayout computeDescriptorSetLayout = nullptr;
	vk::raii::DescriptorPool descriptorPool = nullptr;
	
	AllocatedBuffer uniformBuffer;
	AllocatedBuffer particleBuffer;

	void generateBuffers();
	void createDescriptorSetLayout();
	void createDescriptors();
	void createPipeline();
};
