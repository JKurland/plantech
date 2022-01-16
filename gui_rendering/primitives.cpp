#include "gui_rendering/primitives.h"

#include <utility>
#include <memory>
#include <limits>
#include <iostream>
#include <glm/glm.hpp>

namespace pt {

// static
std::array<VkVertexInputAttributeDescription, 3> TriangleVertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(TriangleVertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(TriangleVertex, colour);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[2].offset = offsetof(TriangleVertex, eventTargetIdx);

    return attributeDescriptions;
}

VertexBufferBuilder::VertexBufferBuilder(const glm::uvec2& screenSize):
    transform(
        2.0/(float)screenSize.x, 0,                       0,
        0,                       2.0/(float)screenSize.y, 0,
        0,                       0,                       1.0
    ),
    translate(-1, -1, 0),
    screenSize(screenSize)
{
}

void VertexBufferBuilder::addRectangle(glm::uvec2 topLeft, glm::uvec2 bottomRight, float depth, glm::vec3 colour, EventTargetHandle eventTargetHandle) {
    const auto topLeft3 = glm::vec3{topLeft.x, topLeft.y, depth};
    const auto bottomRight3 = glm::vec3{bottomRight.x, bottomRight.y, depth};
    const auto bottomLeft3 = glm::vec3{topLeft.x, bottomRight.y, depth};
    const auto topRight3 = glm::vec3{bottomRight.x, topLeft.y, depth};

    assert(eventTargetHandle.idx < std::numeric_limits<uint32_t>::max());
    const uint32_t eventTargetIdx = static_cast<uint32_t>(eventTargetHandle.idx);

    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topLeft3),     .colour = colour, .eventTargetIdx = eventTargetIdx});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomRight3), .colour = colour, .eventTargetIdx = eventTargetIdx});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomLeft3),  .colour = colour, .eventTargetIdx = eventTargetIdx});

    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topRight3),    .colour = colour, .eventTargetIdx = eventTargetIdx});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomRight3), .colour = colour, .eventTargetIdx = eventTargetIdx});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topLeft3),     .colour = colour, .eventTargetIdx = eventTargetIdx});
}

void VertexBufferBuilder::addTriangles(const std::vector<glm::vec3>& points, glm::vec3 colour, const glm::mat4& transform, EventTargetHandle eventTargetHandle) {
    assert(points.size() % 3 == 0);
    assert(eventTargetHandle.idx < std::numeric_limits<uint32_t>::max());
    const uint32_t eventTargetIdx = static_cast<uint32_t>(eventTargetHandle.idx);

    for (const auto& point: points) {
        glm::vec4 withW{point, 1};
        glm::vec4 transformed = transform * withW;
        glm::vec3 pixelSpace{transformed.x, transformed.y, transformed.z};

        triangleVertexBuffer.push_back(TriangleVertex{
            .pos = toScreenSpace(pixelSpace),
            .colour = colour,
            .eventTargetIdx = eventTargetIdx
        });
    }
}

void VertexBufferBuilder::addBackground(float depth, glm::vec3 colour, EventTargetHandle eventTargetHandle) {
    addRectangle(glm::uvec2{0, 0}, screenSize, depth, colour, eventTargetHandle);
}

glm::vec3 VertexBufferBuilder::toScreenSpace(const glm::vec3& x) const {
    return transform * x + translate;
}

VertexBuffers VertexBufferBuilder::build() && {
    return VertexBuffers{
        std::move(triangleVertexBuffer),
        std::move(eventTargets),
    };
}

}