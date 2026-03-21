#pragma once

#include "VulkanInclude.h"
#include "Types.h"

class VulkanContext;
class SwapChainManager;
class GraphicsPipeline;
class ComputePipeline;

class Renderer
{
public:
	Renderer(VulkanContext& context, SwapChainManager& swapChainManager, GraphicsPipeline& graphicsPipeline, ComputePipeline& computePipeline);
	~Renderer();

	void drawFrame();

	void setFrameBufferResized() { framebufferResized = true; }
private:
	VulkanContext& context;
	SwapChainManager& swapChainManager;
	GraphicsPipeline& graphicsPipeline;
	ComputePipeline& computePipeline;

	vk::raii::CommandPool commandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers;

	std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Fence> inFlightFences;

	uint32_t frameIndex = 0;
	bool framebufferResized = false;

	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();
	void recordCommandBuffer(uint32_t imageIndex);
	void transitionImageLayout(
		vk::Image& image,
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout,
		vk::AccessFlags2 srcAccessMask,
		vk::AccessFlags2 dstAccessMask,
		vk::PipelineStageFlags2 srcStageMask,
		vk::PipelineStageFlags2 dstStageMask,
		vk::raii::CommandBuffer& curCommandBuffer
	);

	void createVertexBuffer();

	AllocatedBuffer vertexBuffer;
	uint32_t warmUpFrames = 0;
};

