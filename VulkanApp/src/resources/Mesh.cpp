#include "resources/Mesh.h"

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

    /*vertices = {
        // Face 1 (Front): Normal (0, 0.5, 0.8)
        {{ 0.0f,  0.5f,  0.0f}, {0.8f, 1.0f, 1.0f}, {0.0f, 0.5f, 0.8f}}, // Top
        {{-0.5f, -0.5f,  0.5f}, {0.8f, 1.0f, 1.0f}, {0.0f, 0.5f, 0.8f}}, // Bottom Left
        {{ 0.5f, -0.5f,  0.5f}, {0.8f, 1.0f, 1.0f}, {0.0f, 0.5f, 0.8f}}, // Bottom Right

        // Face 2 (Right): Normal (0.8, 0.5, 0)
        {{ 0.0f,  0.5f,  0.0f}, {0.8f, 1.0f, 1.0f}, {0.8f, 0.5f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.8f, 1.0f, 1.0f}, {0.8f, 0.5f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.8f, 1.0f, 1.0f}, {0.8f, 0.5f, 0.0f}},

        // Face 3 (Back): Normal (0, 0.5, -0.8)
        {{ 0.0f,  0.5f,  0.0f}, {0.8f, 1.0f, 1.0f}, {0.0f, 0.5f, -0.8f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.8f, 1.0f, 1.0f}, {0.0f, 0.5f, -0.8f}},
        {{-0.5f, -0.5f, -0.5f}, {0.8f, 1.0f, 1.0f}, {0.0f, 0.5f, -0.8f}},

        // Face 4 (Left): Normal (-0.8, 0.5, 0)
        {{ 0.0f,  0.5f,  0.0f}, {0.8f, 1.0f, 1.0f}, {-0.8f, 0.5f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.8f, 1.0f, 1.0f}, {-0.8f, 0.5f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.8f, 1.0f, 1.0f}, {-0.8f, 0.5f, 0.0f}}
    };

    indices = {
        0, 1, 2,  3, 4, 5,  6, 7, 8,  9, 10, 11
    };*/

    
    /*vertices = {
        // Face 1 (Front/Y-Positive): Normal (0, 0.707, 0.707)
        {{ 0.0f,  0.0f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.707f, 0.707f}}, // Top
        {{-0.5f,  0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.707f, 0.707f}}, // Left-Back
        {{ 0.5f,  0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.707f, 0.707f}}, // Right-Back

        // Face 2 (Right/X-Positive): Normal (0.707, 0, 0.707)
        {{ 0.0f,  0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.707f, 0.0f, 0.707f}}, // Top
        {{ 0.5f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.707f, 0.0f, 0.707f}}, // Right-Back
        {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.707f, 0.0f, 0.707f}}, // Right-Front

        // Face 3 (Back/Y-Negative): Normal (0, -0.707, 0.707)
        {{ 0.0f,  0.0f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, -0.707f, 0.707f}}, // Top
        {{ 0.5f, -0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -0.707f, 0.707f}}, // Right-Front
        {{-0.5f, -0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -0.707f, 0.707f}}, // Left-Front

        // Face 4 (Left/X-Negative): Normal (-0.707, 0, 0.707)
        {{ 0.0f,  0.0f,  0.5f}, {1.0f, 1.0f, 0.0f}, {-0.707f, 0.0f, 0.707f}}, // Top
        {{-0.5f, -0.5f,  0.0f}, {1.0f, 1.0f, 0.0f}, {-0.707f, 0.0f, 0.707f}}, // Left-Front
        {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 0.0f}, {-0.707f, 0.0f, 0.707f}}  // Left-Back
    };

    indices = {
        0, 2, 1,  3, 5, 4,  6, 8, 7,  9, 11, 10
    };*/

    vertexBuffer = createBuffer(vertices.data(), sizeof(Vertex) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    indexBuffer = createBuffer(indices.data(), sizeof(uint16_t) * indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    indexCount = indices.size();

    setDefaultMaterial();
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