#pragma once

#include "VulkanInclude.h"

class VulkanContext;
class SwapChainManager;
struct AllocatedBuffer;

class ComputePipeline
{
public:
	ComputePipeline(VulkanContext& context, SwapChainManager& swapChainManager);
	~ComputePipeline() = default;
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;
	
	vk::raii::Pipeline computePipeline;
	vk::raii::PipelineLayout pipelineLayout;
	vk::Image targetImage = nullptr;
	vk::raii::ImageView targetImageView = nullptr;
	AllocatedBuffer imageBuffer;

	void createComputeImage();
	void createComputeImageView();
	void createComputePipeline();
};

