#include "renderer/ParticleComputePipeline.h"

#include "shared/EngineConfig.h"
#include "shared/VulkanUtils.h"
#include "shared/FileUtils.h"
#include "core/VulkanContext.h"
#include "core/SwapChainManager.h"
#include "renderer/Particle.h"

#include <random>

ParticleComputePipeline::ParticleComputePipeline(VulkanContext& context, SwapChainManager& swapChainManager)
	: context(context), swapChainManager(swapChainManager) {
	generateBuffers();
	createDescriptorSetLayout();
	createDescriptors();
	createPipeline();
}

ParticleComputePipeline::~ParticleComputePipeline() {
	vmaDestroyBuffer(context.getVmaAllocator(), uniformBuffer.buffer, uniformBuffer.allocation);
	vmaDestroyBuffer(context.getVmaAllocator(), particleBuffer.buffer, particleBuffer.allocation);
}

void ParticleComputePipeline::generateBuffers() {
	uniformBuffer = context.createVmaBuffer(
		sizeof(Snow_UBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO
	);

	std::default_random_engine rndEngine((unsigned)time(nullptr));
	std::uniform_real_distribution<float> xyDist(-4.0f, 4.0f);
	std::uniform_real_distribution<float> zDist(-3.0f, 5.0f);
	std::uniform_real_distribution<float> gravityDist(-0.8f, -0.3f);

	std::vector<Particle> particles(EngineConfig::PARTICLE_COUNT);

	for (auto& particle : particles) {
		particle.position = glm::vec4(xyDist(rndEngine), xyDist(rndEngine), zDist(rndEngine), 1.f);
		particle.velocity = glm::vec4(0.0f, 0.0f, gravityDist(rndEngine), 1.f);
	}

	AllocatedBuffer stagingBuffer = context.createVmaBuffer(
		sizeof(Particle) * EngineConfig::PARTICLE_COUNT,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO
	);

	memcpy(stagingBuffer.info.pMappedData, particles.data(), sizeof(Particle) * EngineConfig::PARTICLE_COUNT);

	particleBuffer = context.createVmaBuffer(
		sizeof(Particle) * EngineConfig::PARTICLE_COUNT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);

	context.copyBuffer(stagingBuffer, particleBuffer, sizeof(Particle) * EngineConfig::PARTICLE_COUNT);

	vmaDestroyBuffer(context.getVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
}

void ParticleComputePipeline::createDescriptorSetLayout() {
	std::array layoutBindings{
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
	};

	vk::DescriptorSetLayoutCreateInfo layoutInfo{ 
		.bindingCount = layoutBindings.size(), 
		.pBindings = layoutBindings.data()
	};

	computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(context.getDevice(), layoutInfo);
}

void ParticleComputePipeline::createDescriptors() {
	std::array<vk::DescriptorPoolSize, 2> poolSizes{
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1)
	};

	vk::DescriptorPoolCreateInfo poolInfo{
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = 1,
		.poolSizeCount = poolSizes.size(),
		.pPoolSizes = poolSizes.data()
	};

	descriptorPool = vk::raii::DescriptorPool(context.getDevice(), poolInfo);

	auto setLayout = *computeDescriptorSetLayout;

	vk::DescriptorSetAllocateInfo allocInfo{
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &setLayout
	};

	computeDescriptorSets = vk::raii::DescriptorSets(context.getDevice(), allocInfo);

	vk::DescriptorBufferInfo uniformBufferInfo(uniformBuffer.buffer, 0, sizeof(Snow_UBO));
	vk::DescriptorBufferInfo storageBufferInfo(particleBuffer.buffer, 0, sizeof(Particle) * EngineConfig::PARTICLE_COUNT);

	std::array<vk::WriteDescriptorSet, 2> descriptorWrites{
		vk::WriteDescriptorSet{
			.dstSet = *computeDescriptorSets[0],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pBufferInfo = &uniformBufferInfo
		},
		vk::WriteDescriptorSet{
			.dstSet = *computeDescriptorSets[0],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pBufferInfo = &storageBufferInfo
		}
	};
	context.getDevice().updateDescriptorSets(descriptorWrites, {});
}

void ParticleComputePipeline::createPipeline() {
	vk::raii::ShaderModule shaderModule = VulkanUtils::createShaderModule(context.getDevice(), FileUtils::readFile(EngineConfig::PARTICLE_COMPUTE_SHADER_PATH));

	vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
		.stage = vk::ShaderStageFlagBits::eCompute,
		.module = shaderModule,
		.pName = "compMain"
	};

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.setLayoutCount = 1,
		.pSetLayouts = &(*computeDescriptorSetLayout),
	};

	pipelineLayout = vk::raii::PipelineLayout(context.getDevice(), pipelineLayoutCreateInfo);

	vk::ComputePipelineCreateInfo pipelineCreateInfo{
		.stage = shaderStageCreateInfo,
		.layout = pipelineLayout
	};

	pipeline = vk::raii::Pipeline(context.getDevice(), nullptr, pipelineCreateInfo);
}

void ParticleComputePipeline::updateParticleUBO(float totalTime, float deltaTime) {
	Snow_UBO computeUBO{};
	computeUBO.totalTime = totalTime;
	computeUBO.deltaTime = deltaTime;

	memcpy(uniformBuffer.info.pMappedData, &computeUBO, sizeof(Snow_UBO));
}