#pragma once

#include "VulkanInclude.h"

#include "Types.h"

class VulkanContext;
class SwapChainManager;

class ComputePipeline
{
public:
	ComputePipeline(VulkanContext& context, SwapChainManager& swapChainManager);
	~ComputePipeline();

	vk::Image& getImage() { return targetImage; }
	vk::raii::ImageView& getImageView() { return targetImageView; }
	vk::raii::Pipeline& getComputePipeline() { return computePipeline; }
	vk::raii::PipelineLayout& getPipelineLayout() { return pipelineLayout; }
	vk::raii::DescriptorSets& getDescriptorSets() { return computeDescriptorSets; };
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;
	
	vk::raii::Pipeline computePipeline = nullptr;
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

