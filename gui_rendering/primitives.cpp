#include "gui_rendering/primitives.h"

#include <utility>
#include <memory>
#include <glm/glm.hpp>

namespace pt {

// static
std::array<VkVertexInputAttributeDescription, 3> TriangleVertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(TriangleVertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(TriangleVertex, colour);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;

    constexpr size_t eventTargetIdxSize = sizeof(std::declval<TriangleVertex>().eventTargetIdx);
    static_assert(eventTargetIdxSize == 4 || eventTargetIdxSize == 8);
    if constexpr (eventTargetIdxSize == 4) {
        attributeDescriptions[2].format = VK_FORMAT_R32_UINT;
    } else if constexpr (eventTargetIdxSize == 8) {
        attributeDescriptions[2].format = VK_FORMAT_R64_UINT;
    }
    attributeDescriptions[2].offset = offsetof(TriangleVertex, eventTargetIdx);

    return attributeDescriptions;
}

VertexBufferBuilder::VertexBufferBuilder(const glm::uvec2& screenSize):
    transform(2.0/(float)screenSize.x, 0, 0, 2.0/(float)screenSize.y),
    translate(-1, -1)
{
}

void VertexBufferBuilder::addRectangle(glm::uvec2 topLeft, glm::uvec2 bottomRight, glm::vec3 colour, EventTargetHandle eventTargetHandle) {
    const auto bottomLeft = glm::uvec2{topLeft.x, bottomRight.y};
    const auto topRight = glm::uvec2{bottomRight.x, topLeft.y};
    const size_t eventTargetIdx = eventTargetHandle.idx;

    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topLeft),     .colour = colour, .eventTargetIdx = eventTargetIdx});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomRight), .colour = colour, .eventTargetIdx = eventTargetIdx});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomLeft),  .colour = colour, .eventTargetIdx = eventTargetIdx});

    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topRight),    .colour = colour, .eventTargetIdx = eventTargetIdx});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(bottomRight), .colour = colour, .eventTargetIdx = eventTargetIdx});
    triangleVertexBuffer.push_back(TriangleVertex{.pos = toScreenSpace(topLeft),     .colour = colour, .eventTargetIdx = eventTargetIdx});
}

glm::vec2 VertexBufferBuilder::toScreenSpace(const glm::uvec2& x) const {
    return transform * x + translate;
}

VertexBuffers VertexBufferBuilder::build() && {
    return VertexBuffers{
        std::move(triangleVertexBuffer),
        std::move(eventTargets),
    };
}

}