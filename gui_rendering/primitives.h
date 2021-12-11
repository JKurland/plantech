#pragma once

#include "gui/gui.h"

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
    std::unique_ptr<void, void(*)(void*)> posData; // extra position information specific to the element type (e.g. which character in a text box)
};

struct TriangleVertex {
    glm::vec2 pos;
    glm::vec3 colour;
    size_t eventTargetIdx;

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

    void addRectangle(glm::uvec2 topLeft, glm::uvec2 bottomRight, glm::vec3 colour, EventTargetHandle eventTargetHandle);

    template<typename T, typename = std::enable_if_t<std::is_void_v<typename T::PosDataT>>>
    EventTargetHandle addEventTarget(const GuiHandle<T>& guiHandle) {
        eventTargets.push_back(EventTarget{
            Gui::convertHandle(guiHandle),
            std::unique_ptr<void, void(*)(void*)>{nullptr, nullptr}
        });
        return EventTargetHandle{eventTargets.size() - 1};
    }

    template<typename T, typename = std::enable_if_t<!std::is_void_v<typename T::PosDataT>>>
    EventTargetHandle addEventTarget(const GuiHandle<T>& guiHandle, typename T::PosDataT posData) {
        eventTargets.push_back(EventTarget{
            Gui::convertHandle(guiHandle),
            std::unique_ptr<void, void(*)(void*)>{new typename T::PosDataT(std::move(posData)), [](void* p){delete static_cast<T::PosDataT*>(p);}}
        });
        return EventTargetHandle{eventTargets.size() - 1};
    }

    VertexBuffers build() &&;
private:
    glm::vec2 toScreenSpace(const glm::uvec2& x) const;

    std::vector<TriangleVertex> triangleVertexBuffer;
    std::vector<EventTarget> eventTargets;

    glm::mat2 transform;
    glm::vec2 translate;
};

}
