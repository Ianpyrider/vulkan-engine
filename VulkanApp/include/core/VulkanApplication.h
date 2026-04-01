#pragma once

class VulkanContext;
class SwapChainManager;
class GraphicsPipeline;
class Renderer;
class ImageComputePipeline;

struct GLFWwindow;
struct GLFWmonitor;

#include <memory>

class VulkanApplication {
public:
	VulkanApplication();
	~VulkanApplication();

	void run();

	void notifyFramebufferResized();
private:
	std::unique_ptr<VulkanContext> vkContext;
	std::unique_ptr<SwapChainManager> vkSwapChainManager;
	std::unique_ptr<GraphicsPipeline> vkGraphicsPipeline;
	std::unique_ptr<Renderer> vkRenderer;
	std::unique_ptr<ImageComputePipeline> vkImageComputePipeline;

	GLFWwindow* window = nullptr;
	GLFWmonitor* monitor = nullptr;

	uint32_t graphicsIndex = ~0;
	const uint32_t WINDOW_WIDTH = 800;
	const uint32_t WINDOW_HEIGHT = 600;

	void mainLoop();
	void initWindow();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};