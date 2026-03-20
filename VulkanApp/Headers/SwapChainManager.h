#pragma once

#include "VulkanInclude.h"

class VulkanContext;
struct GLFWwindow;

class SwapChainManager
{
public:
	SwapChainManager(VulkanContext& context, GLFWwindow* window);
	~SwapChainManager() = default;

	void recreateSwapChain();
	void cleanupSwapChain();

	vk::raii::SwapchainKHR& getSwapChain() { return swapChain; }
	vk::Extent2D getExtent() { return swapChainExtent; }
	std::vector<vk::Image> getImages() { return swapChainImages; }
	std::vector<vk::raii::ImageView>& getImageViews() { return swapChainImageViews; };
	vk::SurfaceFormatKHR getSurfaceFormat() { return swapChainSurfaceFormat; }
private:
	VulkanContext& context;
	GLFWwindow* window;

	vk::raii::SwapchainKHR swapChain = nullptr;

	std::vector<vk::Image> swapChainImages;
	vk::SurfaceFormatKHR swapChainSurfaceFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::raii::ImageView> swapChainImageViews;

	void createSwapChain();
	void createImageViews();
	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
};

