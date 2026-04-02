#include "renderer/ImageComputePipeline.h"

#include <vulkan/vulkan_raii.hpp>

#include  "shared/EngineConfig.h"
#include "shared/FileUtils.h"
#include "shared/VulkanUtils.h"

#include "core/VulkanContext.h"
#include "core/SwapChainManager.h"

ImageComputePipeline::ImageComputePipeline(VulkanContext& context, SwapChainManager& swapChainManager)
	: context(context), swapChainManager(swapChainManager) {
	try {
		createComputeImage();
		createComputeImageView();
		createComputePipeline();
	}
	catch (const std::exception& e) {
		context.destroyVmaImage(targetImage, targetImageAllocation);
		throw;
	}
}
ImageComputePipeline::~ImageComputePipeline() {
	context.destroyVmaImage(targetImage, targetImageAllocation);
}

void ImageComputePipeline::createComputeImage() {
	vk::ImageCreateInfo computeImageInfo{
		.imageType = vk::ImageType::e2D,
		.format = vk::Format::eR8G8B8A8Unorm, // Might have to handle sRGB myself now?
		.extent = {swapChainManager.getExtent().width, swapChainManager.getExtent().height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.priority = 1.0f;

	AllocatedImage createImage = context.createVmaImage(computeImageInfo, allocCreateInfo);
	
	targetImage = createImage.image;
	targetImageAllocation = createImage.allocation;
}

void ImageComputePipeline::createComputeImageView() {
	vk::ImageViewCreateInfo viewInfo{
		.image = targetImage,
		.viewType = vk::ImageViewType::e2D,
		.format = swapChainManager.getSurfaceFormat().format,
		.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};

	targetImageView = vk::raii::ImageView(context.getDevice(), viewInfo);
}

void ImageComputePipeline::createComputePipeline() {
	vk::raii::ShaderModule shaderModule = VulkanUtils::createShaderModule(context.getDevice(), FileUtils::readFile(EngineConfig::IMAGE_COMPUTE_SHADER_PATH));

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

	// Descriptors

	const uint32_t swapChainImageCount = static_cast<uint32_t>(swapChainManager.getImageViews().size());

	// Create Pools
	std::array<vk::DescriptorPoolSize, 2> poolSizes{
		vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, swapChainImageCount},
		vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, swapChainImageCount}
	};

	vk::DescriptorPoolCreateInfo poolInfo{
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = swapChainImageCount,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	computeDescriptorPool = vk::raii::DescriptorPool(context.getDevice(), poolInfo);

	// Allocate descriptor sets

	vk::raii::DescriptorSetLayout computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(context.getDevice(), imageLayoutInfo);

	std::vector<vk::DescriptorSetLayout> layouts;
	layouts.reserve(swapChainImageCount);
	for (uint32_t i = 0; i < swapChainImageCount; i++) {
		layouts.push_back(*computeDescriptorSetLayout);
	}

	vk::DescriptorSetAllocateInfo allocInfo{
		.descriptorPool = computeDescriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(layouts.size()),
		.pSetLayouts = layouts.data()
	};

	computeDescriptorSets = vk::raii::DescriptorSets(context.getDevice(), allocInfo);

	vk::DescriptorImageInfo targetImageDescriptor{
		.imageView = targetImageView,
		.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
	};

	for (size_t i = 0; i < swapChainImageCount; i++) {
		vk::DescriptorImageInfo swapChainImageDescriptor{
			.imageView = swapChainManager.getImageViews()[i],
			.imageLayout = vk::ImageLayout::eGeneral
		};

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites{
			vk::WriteDescriptorSet{
				.dstSet = *computeDescriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eSampledImage,
				.pImageInfo = &targetImageDescriptor
			},

			vk::WriteDescriptorSet{
				.dstSet = *computeDescriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eStorageImage,
				.pImageInfo = &swapChainImageDescriptor
			}
		};

		context.getDevice().updateDescriptorSets(descriptorWrites, nullptr);
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