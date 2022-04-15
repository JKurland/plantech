#pragma once

#include "gui/gui.h"
#include "utils/empty.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>
#include <memory>
#include <array>
#include <type_traits>

namespace pt {

struct EventTarget {
    Gui::Handle handle; // which gui element
    std::shared_ptr<void> posData; // extra position information specific to the element type (e.g. which character in a text box)
};

struct TriangleVertex {
    glm::vec3 pos;
    glm::vec3 colour;
    uint32_t eventTargetIdx;

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct VertexBuffers {
    std::vector<TriangleVertex> triangleVertexBuffer;
    std::vector<EventTarget> eventTargets;
};

class EventTargetHandle {
    EventTargetHandle(size_t idx): idx(idx) {}
    size_t idx;
    friend class VertexBufferBuilder;
};

// all arguments to these methods are in pixel
class VertexBufferBuilder {
public:
    VertexBufferBuilder(const glm::uvec2& screeSize);

    void addRectangle(glm::uvec2 topLeft, glm::uvec2 bottomRight, float depth, glm::vec3 colour, EventTargetHandle eventTargetHandle);
    void addBackground(float depth, glm::vec3 colour, EventTargetHandle eventTargetHandle);

    // applies transform to each point in points and then interprets the points
    // as a triangle list (i.e. points.size()%3 == 0). Transform should transform
    // the points into pixel space.
    void addTriangles(const std::vector<glm::vec3>& points, glm::vec3 colour, const glm::mat4& transform, EventTargetHandle eventTargetHandle);

    template<typename T, typename = std::enable_if_t<std::is_same_v<typename T::PosDataT, Empty>>>
    EventTargetHandle addEventTarget(const GuiHandle<T>& guiHandle) {
        eventTargets.push_back(EventTarget{
            Gui::convertHandle(guiHandle),
            nullptr
        });
        return EventTargetHandle{eventTargets.size() - 1};
    }

    template<typename T, typename = std::enable_if_t<!std::is_same_v<typename T::PosDataT, Empty>>>
    EventTargetHandle addEventTarget(const GuiHandle<T>& guiHandle, typename T::PosDataT posData) {
        eventTargets.push_back(EventTarget{
            Gui::convertHandle(guiHandle),
            std::make_shared(posData)
        });
        return EventTargetHandle{eventTargets.size() - 1};
    }

    VertexBuffers build() &&;
private:
    glm::vec3 toScreenSpace(const glm::vec3& x) const;

    std::vector<TriangleVertex> triangleVertexBuffer;
    std::vector<EventTarget> eventTargets;

    glm::mat3 transform;
    glm::vec3 translate;
    glm::uvec2 screenSize;
};

}
