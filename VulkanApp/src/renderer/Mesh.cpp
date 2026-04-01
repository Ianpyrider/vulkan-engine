#include "renderer/Mesh.h"

#include "core/VulkanContext.h"

#include <tiny_obj_loader/tiny_obj_loader.h>

//const std::vector<Vertex> vertices = {
//    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
//    {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
//    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
//    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
//
//    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
//    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
//    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
//    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}}
//};
//
//const std::vector<uint16_t> indices = {
//    0, 1, 2, 2, 3, 0,
//    4, 5, 6, 6, 7, 4
//};

Mesh::Mesh(const std::string &filepath, VulkanContext& context) : context(context) {
    loadFromObj(filepath);

    vertexBuffer = createBuffer(vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = createBuffer(indices.data(), sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    indexCount = indices.size();
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

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }
 }