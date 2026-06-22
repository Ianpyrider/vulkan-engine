#pragma once

#include "shared/VulkanInclude.h"
#include "shared/Types.h"

class Scene
{
public:
	Scene();
	~Scene() = default;

private:
	AllocatedImage textureImage;
	vk::raii::ImageView textureImageView = nullptr;
	vk::raii::Sampler textureSampler = nullptr;
	std::vector<vk::raii::DescriptorSet> descriptorSets;
	vk::Format textureFormat = vk::Format::eR8G8B8A8Unorm;
};
