#pragma once

#include "shared/VulkanInclude.h"
#include "shared/Types.h"
#include "renderer/Mesh.h"

#include <chrono>

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

	// Scene objects
	Mesh triangleMesh;

	// Command buffer
	vk::raii::CommandPool commandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers;

	// Synchronization
	uint32_t frameIndex = 0;
	
	std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
	std::vector<vk::raii::Fence> inFlightFences;

	// Timestamp queries
	const uint32_t numDeltas = 60;

	std::vector<float> frameDeltas = std::vector<float>(numDeltas);
	uint32_t frameDeltasI = 0;
	
	float timeSinceLastPrint = 0.0;
	std::chrono::time_point<std::chrono::steady_clock> prevFrameTime;

	// UBO
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	std::vector<AllocatedBuffer> uniformBuffers;

	vk::raii::DescriptorPool descriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;

	// Resizing support
	bool framebufferResized = false;

	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();

	void updateUniformBuffer(uint32_t frameIndex, float timeElapsed);
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

	uint32_t warmUpFrames = 0;
};

