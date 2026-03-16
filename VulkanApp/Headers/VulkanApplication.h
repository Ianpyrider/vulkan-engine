#pragma once

class VulkanContext;
class SwapChainManager;
class GraphicsPipeline;
class Renderer;

struct GLFWwindow;

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

	GLFWwindow* window = nullptr;

	uint32_t graphicsIndex = ~0;

	void mainLoop();
	void initWindow();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};