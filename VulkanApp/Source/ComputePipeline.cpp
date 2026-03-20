#include "ComputePipeline.h"

#include <vulkan/vulkan_raii.hpp>

#include "engineConfig.h"
#include "fileUtils.h"
#include "vulkanUtils.h"

#include "VulkanContext.h"
#include "SwapChainManager.h"

ComputePipeline::ComputePipeline(VulkanContext& context, SwapChainManager& swapChainManager) : context(context), swapChainManager(swapChainManager) {
	createComputeImage();
	createComputeImageView();
	createComputePipeline();
}

void ComputePipeline::createComputeImage() {
	vk::ImageCreateInfo computeImageInfo{
		.imageType = vk::ImageType::e2D,
		.format = swapChainManager.getSurfaceFormat().format,
		.extent = {swapChainManager.getExtent().width, swapChainManager.getExtent().height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.priority = 1.0f;

	targetImage = context.createVmaImage(computeImageInfo, allocCreateInfo);
}

void ComputePipeline::createComputeImageView() {
	vk::ImageViewCreateInfo viewInfo{
		.image = targetImage,
		.viewType = vk::ImageViewType::e2D,
		.format = swapChainManager.getSurfaceFormat().format,
		.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};

	targetImageView = vk::raii::ImageView(context.getDevice(), viewInfo);
}

void ComputePipeline::createComputePipeline() {
	vk::raii::ShaderModule shaderModule = vulkanUtils::createShaderModule(context.getDevice(), fileUtils::readFile(engineConfig::SHADER_PATH));

	vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{
		.stage = vk::ShaderStageFlagBits::eCompute,
		.module = shaderModule,
		.pName = "compMain"
	};

	vk::DescriptorSetLayoutBinding inputBinding{
		.binding = 0,
		.descriptorType = vk::DescriptorType::eSampledImage,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eCompute
	};

	vk::DescriptorSetLayoutBinding outputBinding{
		.binding = 1,
		.descriptorType = vk::DescriptorType::eStorageImage,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eCompute
	};

	std::array imageLayoutBindings{
		inputBinding,
		outputBinding
	};

	vk::DescriptorSetLayoutCreateInfo imageLayoutInfo{
		.bindingCount = imageLayoutBindings.size(),
		.pBindings = imageLayoutBindings.data()
	};

	vk::raii::DescriptorSetLayout computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(context.getDevice(), imageLayoutInfo);

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{
		.setLayoutCount = 1,
		.pSetLayouts = &(*computeDescriptorSetLayout),
	};

	pipelineLayout = vk::raii::PipelineLayout(context.getDevice(), pipelineLayoutCreateInfo);

	vk::ComputePipelineCreateInfo pipelineCreateInfo{
		.stage = shaderStageCreateInfo,
		.layout = pipelineLayout
	};

	computePipeline = vk::raii::Pipeline(context.getDevice(), nullptr, pipelineCreateInfo);
}