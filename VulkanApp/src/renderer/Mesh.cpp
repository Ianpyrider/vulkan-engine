#include "renderer/Mesh.h"

#include "core/VulkanContext.h"

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

Mesh::Mesh(VulkanContext& context) : context(context) {
    indexCount = indices.size();
    vertexBuffer = createBuffer(vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = createBuffer(indices.data(), sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

Mesh::~Mesh() {
    vmaDestroyBuffer(context.getVmaAllocator(), vertexBuffer.buffer, vertexBuffer.allocation);
    vmaDestroyBuffer(context.getVmaAllocator(), indexBuffer.buffer, indexBuffer.allocation);
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