#pragma once

#include "shared/VulkanInclude.h"

#include "shared/Types.h"

class VulkanContext;
class SwapChainManager;

class ImageComputePipeline
{
public:
	ImageComputePipeline(VulkanContext& context, SwapChainManager& swapChainManager);
	~ImageComputePipeline();

	vk::Image& getImage() { return targetImage; }
	vk::raii::ImageView& getImageView() { return targetImageView; }
	vk::raii::Pipeline& getComputePipeline() { return pipeline; }
	vk::raii::PipelineLayout& getPipelineLayout() { return pipelineLayout; }
	vk::raii::DescriptorSets& getDescriptorSets() { return computeDescriptorSets; };
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;
	
	vk::raii::Pipeline pipeline = nullptr;
	vk::raii::PipelineLayout pipelineLayout = nullptr;

	vk::raii::ImageView targetImageView = nullptr;
	vk::Image targetImage = nullptr;
	VmaAllocation targetImageAllocation;
	AllocatedBuffer imageBuffer;

	vk::raii::DescriptorPool computeDescriptorPool = nullptr;
	vk::raii::DescriptorSets computeDescriptorSets{ nullptr };

	void createComputeImage();
	void createComputeImageView();
	void createComputePipeline();
};

