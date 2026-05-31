#pragma once

#include "shared/VulkanInclude.h"

class VulkanContext;
class SwapChainManager;

class GraphicsPipeline
{
public:
	struct PipelineConfig {
		std::string shaderPath;
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
		vk::PrimitiveTopology topology;
		std::vector<vk::PushConstantRange> pushConstantRanges = {};
	};

	GraphicsPipeline(VulkanContext& context, SwapChainManager& swapChainManager, const PipelineConfig& config);
	~GraphicsPipeline() = default;

	vk::raii::Pipeline& getGraphicsPipeline() { return graphicsPipeline; }
	vk::raii::PipelineLayout& getPipelineLayout() { return pipelineLayout; }
	vk::raii::DescriptorSetLayout& getDescriptorSetLayout() { return descriptorSetLayout; };
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;
	const PipelineConfig& config;

	vk::raii::PipelineLayout pipelineLayout = nullptr;
	vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
	vk::raii::Pipeline graphicsPipeline = nullptr;

	void createGraphicsPipeline();
	void createDescriptorSetLayout();
};

