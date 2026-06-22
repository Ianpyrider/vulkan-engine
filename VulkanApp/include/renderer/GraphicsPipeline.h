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
	vk::raii::DescriptorSetLayout& getDescriptorSetLayout(uint32_t i) { return descriptorSetLayouts[i]; };

	enum DescriptorSetSlot : uint32_t {
		Global = 0,		// Set 0 (Camera/UBO)
		IBL = 1,		// Set 1 (Textures)
	};
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;
	const PipelineConfig& config;

	vk::raii::PipelineLayout pipelineLayout = nullptr;
	std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayouts = {};
	vk::raii::Pipeline graphicsPipeline = nullptr;

	void createGraphicsPipeline();
	void createDescriptorSetLayouts();
};

