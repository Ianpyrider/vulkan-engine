#include "resources/Mesh.h"

#include "shared/EngineConfig.h"
#include "core/VulkanContext.h"
#include "core/SwapChainManager.h"
#include "renderer/GraphicsPipeline.h"

#include <tiny_obj_loader/tiny_obj_loader.h>
#include <stb/stb_image.h>

Mesh::Mesh(
    const std::string &filepath, 
    VulkanContext& context,
    SwapChainManager& swapChainManager,
    GraphicsPipeline& graphicsPipeline,
    vk::raii::DescriptorPool& descriptorPool) : 
        context(context), 
        swapChainManager(swapChainManager), 
        graphicsPipeline(graphicsPipeline)
    {
    //loadFromObj(filepath);

    vertices = {
        // Position                 // Color                    // Normal                  // UV
        {{-0.5f, -0.5f,  0.0f},     {1.0f, 0.0f, 0.0f},         {0.0f, 0.0f, 1.0f},        {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.0f},     {1.0f, 0.0f, 0.0f},         {0.0f, 0.0f, 1.0f},        {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.0f},     {1.0f, 1.0f, 1.0f},         {0.0f, 0.0f, 1.0f},        {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.0f},     {1.0f, 1.0f, 1.0f},         {0.0f, 0.0f, 1.0f},        {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f},     {1.0f, 0.0f, 0.0f},         {0.0f, 0.0f, -1.0f},       {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f},     {0.0f, 1.0f, 0.0f},         {0.0f, 0.0f, -1.0f},       {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f},     {0.0f, 0.0f, 1.0f},         {0.0f, 0.0f, -1.0f},       {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f},     {1.0f, 1.0f, 1.0f},         {0.0f, 0.0f, -1.0f},       {1.0f, 1.0f}}
    };

    indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    vertexBuffer = createBuffer(vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = createBuffer(indices.data(), sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    textureImage = createImage("assets/textures/ernesto.png");
    
    createImageView();
    createTextureSampler();

    createMeshDescriptorSet(descriptorPool);
    
    indexCount = indices.size();

    setDefaultMaterial();
}

Mesh::~Mesh() {
    vmaDestroyBuffer(context.getVmaAllocator(), vertexBuffer.buffer, vertexBuffer.allocation);
    vmaDestroyBuffer(context.getVmaAllocator(), indexBuffer.buffer, indexBuffer.allocation);
    context.destroyVmaImage(textureImage.image, textureImage.allocation);
}

AllocatedBuffer Mesh::createBuffer(const void* data, size_t bufferSize, VkBufferUsageFlags usage) {
    AllocatedBuffer stagingBuffer = context.createVmaBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );

    memcpy(stagingBuffer.info.pMappedData, data, bufferSize);

    AllocatedBuffer outputBuffer = context.createVmaBuffer(
        bufferSize,
        usage,
        0,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    );

    context.copyBuffer(stagingBuffer, outputBuffer, bufferSize);

    vmaDestroyBuffer(context.getVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);

    return outputBuffer;
}

AllocatedImage Mesh::createImage(const char* src) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(src, & texWidth, & texHeight, & texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    AllocatedBuffer stagingBuffer = context.createVmaBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );

    void* data = stagingBuffer.info.pMappedData;
    memcpy(data, pixels, imageSize);

    stbi_image_free(pixels);

    vk::ImageCreateInfo imageInfo{ .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR8G8B8A8Unorm,
        .extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive
    };

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocCreateInfo.priority = 1.0f;

    AllocatedImage image = context.createVmaImage(imageInfo, allocCreateInfo);

    vk::raii::CommandBuffer commandBuffer = context.beginSingleTimeCommands();

    context.transitionImageLayout(
        image.image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        vk::AccessFlagBits2::eNone,                     // srcAccessMask
        vk::AccessFlagBits2::eTransferWrite,            // dstAccessMask
        vk::PipelineStageFlagBits2::eTopOfPipe,         // srcStage
        vk::PipelineStageFlagBits2::eTransfer,          // dstStage
        vk::ImageAspectFlagBits::eColor,
        commandBuffer
    );

    context.copyBufferToImage(commandBuffer, stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    context.transitionImageLayout(
        image.image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite,                     // srcAccessMask
        vk::AccessFlagBits2::eShaderRead,            // dstAccessMask
        vk::PipelineStageFlagBits2::eTransfer,         // srcStage
        vk::PipelineStageFlagBits2::eFragmentShader,          // dstStage
        vk::ImageAspectFlagBits::eColor,
        commandBuffer
    );

    context.endSingleTimeCommands(std::move(commandBuffer));

    vmaDestroyBuffer(context.getVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);

    return image;
}

void Mesh::createImageView() {
    vk::ImageViewCreateInfo viewInfo{
        .image = textureImage.image,
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainManager.getSurfaceFormat().format,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
    };

    textureImageView = vk::raii::ImageView(context.getDevice(), viewInfo);
}

void Mesh::createTextureSampler() {
    vk::PhysicalDeviceProperties properties = context.getPhysicalDevice().getProperties();

    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways
    };

    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = vk::False;
    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    textureSampler = vk::raii::Sampler(context.getDevice(), samplerInfo);
}

void Mesh::createMeshDescriptorSet(vk::raii::DescriptorPool& descriptorPool) {
    std::vector<vk::DescriptorSetLayout> layouts;
    layouts.reserve(EngineConfig::MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < EngineConfig::MAX_FRAMES_IN_FLIGHT; i++) {
        layouts.push_back(*graphicsPipeline.getDescriptorSetLayout(EngineConfig::DescriptorSetSlot::Mesh));
    }

    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = *descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    descriptorSets = vk::raii::DescriptorSets(context.getDevice(), allocInfo);

    for (uint32_t i = 0; i < EngineConfig::MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorImageInfo imageInfo{ 
            .sampler = textureSampler, 
            .imageView = textureImageView, 
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet descriptorWrite{
            .dstSet = *descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        };

        context.getDevice().updateDescriptorSets(descriptorWrite, nullptr);
    }
}

void Mesh::loadFromObj(const std::string& filePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str())) {
        throw std::runtime_error("Caught runtime error: " + warn + err);
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (attrib.colors.empty()) {
                throw std::runtime_error("no vertex colors");
            }

            vertex.color = {
                attrib.colors[3 * index.vertex_index + 0],
                attrib.colors[3 * index.vertex_index + 1],
                attrib.colors[3 * index.vertex_index + 2]
            };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }
 }