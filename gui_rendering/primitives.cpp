#include "gui_rendering/primitives.h"

#include <glm/glm.hpp>

namespace pt {

// static
std::array<VkVertexInputAttributeDescription, 2> TriangleVertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(TriangleVertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(TriangleVertex, colour);

    return attributeDescriptions;
}

VertexBufferBuilder::VertexBufferBuilder(const glm::uvec2& screenSize):
    transform(2.0/(float)screenSize.x, 0, 0, 2.0/(float)screenSize.y),
    translate(-1, -1)
{
}

void VertexBufferBuilder::addRectangle(glm::uvec2 topLeft, glm::uvec2 bottomRight, glm::vec3 colour) {
    const auto bottomLeft = glm::uvec2{topLeft.x, bottomRight.y};
    const auto topRight = glm::uvec2{bottomRight.x, topLeft.y};

    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topLeft),     .colour = colour});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomRight), .colour = colour});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomLeft),  .colour = colour});

    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topRight),    .colour = colour});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomRight), .colour = colour});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topLeft),     .colour = colour});
}

glm::vec2 VertexBufferBuilder::toScreenSpace(const glm::uvec2& x) const {
    return transform * x + translate;
}

std::vector<TriangleVertex> VertexBufferBuilder::getTriangleVertices() {
    return triangleVertexBuffer;
}

}