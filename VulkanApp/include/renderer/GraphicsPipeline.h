#pragma once

#include "shared/VulkanInclude.h"

class VulkanContext;
class SwapChainManager;

class GraphicsPipeline
{
public:
	GraphicsPipeline(VulkanContext& context, SwapChainManager& swapChainManager);
	~GraphicsPipeline() = default;

	vk::raii::Pipeline& getGraphicsPipeline() { return graphicsPipeline; }
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;

	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline = nullptr;

	void createGraphicsPipeline();
};

