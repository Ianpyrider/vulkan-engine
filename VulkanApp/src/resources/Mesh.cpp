#include "resources/Mesh.h"

#include <iostream>

#include "shared/EngineConfig.h"
#include "core/VulkanContext.h"
#include "core/SwapChainManager.h"
#include "renderer/GraphicsPipeline.h"

#include <ktx/ktx.h>
#include <ktx/ktxvulkan.h>

Mesh::Mesh(
    const std::string& filepath,
    VulkanContext& context,
    SwapChainManager& swapChainManager,
    GraphicsPipeline& graphicsPipeline,
    vk::raii::DescriptorPool& descriptorPool) :
    context(context),
    swapChainManager(swapChainManager),
    graphicsPipeline(graphicsPipeline)
{

    //loadFromGltf(filepath);

    vertices = {
        // Position                 // Color                    // Normal                  // UV
        {{-0.5f, -0.5f,  0.5f},     {1.0f, 0.0f, 0.0f},         {0.0f, 0.0f, 1.0f},        {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f},     {1.0f, 0.0f, 0.0f},         {0.0f, 0.0f, 1.0f},        {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f},     {1.0f, 1.0f, 1.0f},         {0.0f, 0.0f, 1.0f},        {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f},     {1.0f, 1.0f, 1.0f},         {0.0f, 0.0f, 1.0f},        {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f},     {1.0f, 0.0f, 0.0f},         {0.0f, 0.0f, -1.0f},       {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f},     {0.0f, 1.0f, 0.0f},         {0.0f, 0.0f, -1.0f},       {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f},     {0.0f, 0.0f, 1.0f},         {0.0f, 0.0f, -1.0f},       {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f},     {1.0f, 1.0f, 1.0f},         {0.0f, 0.0f, -1.0f},       {1.0f, 1.0f}}
    };

    indices = {
        // Top face
        0, 1, 2,
        2, 3, 0,

        // Bottom face
        5, 4, 7,
        7, 6, 5,

        4, 0, 3,
        3, 7, 4,

        1, 5, 6,
        6, 2, 1,

        3, 2, 6,
        6, 7, 3,

        4, 5, 1,
        1, 0, 4
    };

    vertexBuffer = createBuffer(vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = createBuffer(indices.data(), sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    //std::vector<unsigned char> solidPixel = { static_cast<unsigned char>(255.0), static_cast<unsigned char>(255.0), static_cast<unsigned char>(0.0), static_cast<unsigned char>(255.0) };
    //textureImage = createImage(solidPixel.data(), 1, 1, 4, 4, vk::Format::eR8G8B8A8Unorm);

    createImageFromKTXFile("assets/textures/forest_irradiance.ktx");
    createImageView();
    createTextureSampler();
    createMeshDescriptorSet(descriptorPool);
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

void Mesh::createImageFromKTXFile(std::string filename) {
    // Load KTX2 texture instead of using stb_image
    ktxTexture* kTexture;
    KTX_error_code result = ktxTexture_CreateFromNamedFile(
        filename.c_str(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &kTexture);

    if (result != KTX_SUCCESS) {
        throw std::runtime_error("failed to load ktx texture image!");
    }

    uint32_t texWidth = kTexture->baseWidth;
    uint32_t texHeight = kTexture->baseHeight;
    ktx_size_t imageSize = ktxTexture_GetDataSize(kTexture);
    ktx_uint8_t* ktxTextureData = ktxTexture_GetData(kTexture);
    
    textureFormat = static_cast<vk::Format>(ktxTexture_GetVkFormat(kTexture));

    // Create Image!
    AllocatedBuffer stagingBuffer = context.createVmaBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_AUTO
    );

    void* data = stagingBuffer.info.pMappedData;
    memcpy(data, ktxTextureData, imageSize);

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = textureFormat,
        .extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
        .mipLevels = 1,
        .arrayLayers = 6,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive,
    };

    imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;

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
        commandBuffer,
        6
    );

    std::vector<vk::BufferImageCopy> copyRegions;
    copyRegions.reserve(6);

    for (int i = 0; i < 6; i++) {
        size_t offset;
        ktxTexture_GetImageOffset(kTexture, 0, 0, i, &offset);

        vk::BufferImageCopy region{};
        region.bufferOffset = offset;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = static_cast<uint32_t>(i);
        region.imageSubresource.layerCount = 1;

        region.imageOffset = vk::Offset3D{ 0, 0, 0 };
        region.imageExtent = vk::Extent3D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };

        copyRegions.push_back(region);
    }

    commandBuffer.copyBufferToImage(stagingBuffer.buffer, image.image, vk::ImageLayout::eTransferDstOptimal, copyRegions);

    context.transitionImageLayout(
        image.image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits2::eTransferWrite,                     // srcAccessMask
        vk::AccessFlagBits2::eShaderRead,            // dstAccessMask
        vk::PipelineStageFlagBits2::eTransfer,         // srcStage
        vk::PipelineStageFlagBits2::eFragmentShader,          // dstStage
        vk::ImageAspectFlagBits::eColor,
        commandBuffer,
        6
    );

    context.endSingleTimeCommands(std::move(commandBuffer));

    textureImage = image;

    vmaDestroyBuffer(context.getVmaAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
    ktxTexture_Destroy(kTexture);
}

// TODO: Consolidate this with ktx image, ideally make a general staging function in Context? Ignoring DRY for clarity before cleanup
AllocatedImage Mesh::createImage(unsigned char* pixels, int texWidth, int texHeight, int texChannels) {
    vk::DeviceSize imageSize = texWidth * texHeight * texChannels;

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

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = textureFormat,
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

    vk::BufferImageCopy copyRegions{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1}
    };

    commandBuffer.copyBufferToImage(stagingBuffer.buffer, image.image, vk::ImageLayout::eTransferDstOptimal, copyRegions);

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
        .viewType = vk::ImageViewType::eCube,
        .format = textureFormat,
        .subresourceRange = vk::ImageSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 6}) // Last is layercount
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

void Mesh::loadFromGltf(const std::string& filepath) {
    // Use tinygltf to load the model instead of tinyobjloader
    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    std::string        err;
    std::string        warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);

    if (!warn.empty())
    {
        std::cout << "glTF warning: " << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cout << "glTF error: " << err << std::endl;
    }

    if (!ret)
    {
        throw std::runtime_error("Failed to load glTF model");
    }

    vertices.clear();
    indices.clear();

    // Process all meshes in the model
    for (const auto& mesh : model.meshes)
    {
        for (const auto& primitive : mesh.primitives)
        {
            // Get indices
            const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];

            // Get vertex positions
            const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
            const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
            const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];

            // Get vertex normals
            const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];;
            const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
            const tinygltf::Buffer& normBuffer = model.buffers[normBufferView.buffer];

            // Get texture coordinates if available
            bool                        hasTexCoords = primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end();
            const tinygltf::Accessor* texCoordAccessor = nullptr;
            const tinygltf::BufferView* texCoordBufferView = nullptr;
            const tinygltf::Buffer* texCoordBuffer = nullptr;

            // Get colors if available
            bool                        hasColor = primitive.attributes.find("COLOR_0") != primitive.attributes.end();
            const tinygltf::Accessor* colCoordAccessor = nullptr;
            const tinygltf::BufferView* colCoordBufferView = nullptr;
            const tinygltf::Buffer* colCoordBuffer = nullptr;

            if (hasTexCoords)
            {
                texCoordAccessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
                texCoordBufferView = &model.bufferViews[texCoordAccessor->bufferView];
                texCoordBuffer = &model.buffers[texCoordBufferView->buffer];
            }

            if (hasColor)
            {
                colCoordAccessor = &model.accessors[primitive.attributes.at("COLOR_0")];
                colCoordBufferView = &model.bufferViews[colCoordAccessor->bufferView];
                colCoordBuffer = &model.buffers[colCoordBufferView->buffer];
            }

            uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

            for (size_t i = 0; i < posAccessor.count; i++)
            {
                Vertex vertex{};

                size_t posStride = posBufferView.byteStride == 0 ? 12 : posBufferView.byteStride;
                size_t normStride = normBufferView.byteStride == 0 ? 12 : normBufferView.byteStride;

                size_t texStride = hasTexCoords ? (texCoordBufferView->byteStride == 0 ? 8 : texCoordBufferView->byteStride) : 0;

                // Colors can be exported as VEC3 (12 bytes) or VEC4 (16 bytes)
                size_t colorStride = 0;
                if (hasColor) {
                    colorStride = colCoordBufferView->byteStride == 0 ?
                        (colCoordAccessor->type == TINYGLTF_TYPE_VEC3 ? 12 : 16) : colCoordBufferView->byteStride;
                }

                // Vulkan uses Y-down but gltf uses Y-up so we flip Y
                const float* pos = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset + i * posStride]);
                vertex.pos = { pos[0], pos[1], pos[2] };

                const float* normal = reinterpret_cast<const float*>(&normBuffer.data[normBufferView.byteOffset + normAccessor.byteOffset + i * normStride]);
                vertex.normal = { normal[0], normal[1], normal[2] };

                if (hasTexCoords)
                {
                    const float* texCoord = reinterpret_cast<const float*>(&texCoordBuffer->data[texCoordBufferView->byteOffset + texCoordAccessor->byteOffset + i * 8]);
                    vertex.texCoord = { texCoord[0], texCoord[1] };
                }
                else
                {
                    vertex.texCoord = { 0.0f, 0.0f };
                }

                if (hasColor) {
                    if (colCoordAccessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                        const float* color = reinterpret_cast<const float*>(&colCoordBuffer->data[colCoordBufferView->byteOffset + colCoordAccessor->byteOffset + i * colorStride]);

                        vertex.color = { color[0], color[1], color[2] };

                        // float alpha = (colorAccessor->type == TINYGLTF_TYPE_VEC4) ? color[3] : 1.0f;
                    }
                }
                else {
                    // Red fallback for debugging
                    vertex.color = { 1.0f, 0.0f, 0.0f };
                }
                vertices.push_back(vertex);
            }

            const unsigned char* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
            size_t               indexCount = indexAccessor.count;
            size_t               indexStride = 0;

            // Determine index stride based on component type
            if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                indexStride = sizeof(uint16_t);
            }
            else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
            {
                indexStride = sizeof(uint32_t);
            }
            else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                indexStride = sizeof(uint8_t);
            }
            else
            {
                throw std::runtime_error("Unsupported index component type");
            }

            indices.reserve(indices.size() + indexCount);

            for (size_t i = 0; i < indexCount; i++)
            {
                uint32_t index = 0;

                if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    index = *reinterpret_cast<const uint16_t*>(indexData + i * indexStride);
                }
                else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    index = *reinterpret_cast<const uint32_t*>(indexData + i * indexStride);
                }
                else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    index = *reinterpret_cast<const uint8_t*>(indexData + i * indexStride);
                }

                indices.push_back(baseVertex + index);
            }
        }
    }

    bool textureLoaded = false;

    if (!model.meshes.empty() && !model.meshes[0].primitives.empty()) {
        int materialIndex = model.meshes[0].primitives[0].material;

        if (materialIndex >= 0) {

            setMaterial(model.materials[materialIndex]);
            
            int textureIndex = model.materials[materialIndex].pbrMetallicRoughness.baseColorTexture.index;

            if (textureIndex >= 0) {
                int imageIndex = model.textures[textureIndex].source;
                tinygltf::Image& gltfImage = model.images[imageIndex];

                textureImage = createImage(
                    gltfImage.image.data(),
                    gltfImage.width,
                    gltfImage.height,
                    gltfImage.component
                );
            } else {
                std::vector<double> baseColor = model.materials[materialIndex].pbrMetallicRoughness.baseColorFactor;

                std::vector<unsigned char> solidPixel = {
                    static_cast<unsigned char>(baseColor[0] * 255.0),
                    static_cast<unsigned char>(baseColor[1] * 255.0),
                    static_cast<unsigned char>(baseColor[2] * 255.0),
                    static_cast<unsigned char>(baseColor[3] * 255.0)
                };

                textureImage = createImage(solidPixel.data(), 1, 1, 4);
            }

            textureLoaded = true;
        }
    }

    if (!textureLoaded)
    {
        throw std::runtime_error("Failed to load gltf texture for imported mesh");
    }
}

void Mesh::setMaterial(const tinygltf::Material& gltfMaterial) {
    pbrPushConstants.baseColorFactor = glm::vec4(
        gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
        gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
        gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
        gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]
    );
    pbrPushConstants.metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
    pbrPushConstants.roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

    pbrPushConstants.metallicFactor = 1.f;
    pbrPushConstants.roughnessFactor = 0.5f;

    pbrPushConstants.baseColorTextureSet = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index > -1
        ? gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord : -1;

    pbrPushConstants.physicalDescriptorTextureSet = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index > -1
        ? gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord : -1;

    pbrPushConstants.normalTextureSet = gltfMaterial.normalTexture.index > -1
        ? gltfMaterial.normalTexture.texCoord : -1;

    pbrPushConstants.occlusionTextureSet = gltfMaterial.occlusionTexture.index > -1
        ? gltfMaterial.occlusionTexture.texCoord : -1;

    pbrPushConstants.emissiveTextureSet = gltfMaterial.emissiveTexture.index > -1
        ? gltfMaterial.emissiveTexture.texCoord : -1;

    pbrPushConstants.alphaMask = (gltfMaterial.alphaMode == "MASK") ? 1.0f : 0.0f;
    pbrPushConstants.alphaMaskCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
}