#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>
#include <array>

namespace pt {

struct TriangleVertex {
    glm::vec2 pos;
    glm::vec3 colour;

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

// all arguments to these methods are in pixel
class VertexBufferBuilder {
public:
    VertexBufferBuilder(const glm::uvec2& screeSize);

    void addRectangle(glm::uvec2 topLeft, glm::uvec2 bottomRight, glm::vec3 colour);

    std::vector<TriangleVertex> getTriangleVertices();
private:
    glm::vec2 toScreenSpace(const glm::uvec2& x) const;

    std::vector<TriangleVertex> triangleVertexBuffer;
    glm::mat2 transform;
    glm::vec2 translate;
};

}
