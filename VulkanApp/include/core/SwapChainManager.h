#pragma once

#include "shared/VulkanInclude.h"

#include "shared/Types.h"

class VulkanContext;
struct GLFWwindow;

class SwapChainManager
{
public:
	SwapChainManager(VulkanContext& context, GLFWwindow* window);
	~SwapChainManager();

	void recreateSwapChain();
	void cleanupSwapChain();

	vk::raii::SwapchainKHR& getSwapChain() { return swapChain; }
	vk::Extent2D getExtent() { return swapChainExtent; }
	std::vector<vk::Image> getImages() { return swapChainImages; }
	std::vector<vk::raii::ImageView>& getImageViews() { return swapChainImageViews; }
	vk::SurfaceFormatKHR getSurfaceFormat() { return swapChainSurfaceFormat; }
	vk::raii::ImageView& getDepthImageView() { return depthImageView; }
	AllocatedImage& getDepthImage() { return depthImage; }
	vk::Format& getDepthFormat() { return depthFormat; };
private:
	VulkanContext& context;
	GLFWwindow* window;

	vk::raii::SwapchainKHR swapChain = nullptr;

	std::vector<vk::Image> swapChainImages;
	vk::SurfaceFormatKHR swapChainSurfaceFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::raii::ImageView> swapChainImageViews;

	AllocatedImage depthImage;
	vk::raii::ImageView depthImageView = nullptr;
	vk::Format depthFormat;

	void createSwapChain();
	void createImageViews();
	void createDepthResources();

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
};

