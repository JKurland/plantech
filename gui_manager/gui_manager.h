#pragma once

#include "framework/context.h"
#include "gui_rendering/gui_renderer.h"
#include "window/window.h"
#include "gui/button.h"

#include <optional>

namespace pt {

struct AddButton {
    using ResponseT = GuiHandle<Button>;
};

template<typename T>
struct AddGuiElement {
    using ResponseT = GuiHandle<T>;

    // if nullopt parent is gui.root()
    std::optional<Gui::Handle> parent;
    T element;
};


class GuiManager {
public:
    EVENT(MouseButton) {
        auto req = GetEventTargetForPixel{static_cast<uint32_t>(event.x), static_cast<uint32_t>(event.y)};
        EventTarget target = co_await ctx(req);
        if (event.action == GLFW_PRESS) {
            gui.mouseButtonDown(target.handle);
        } else {
            gui.mouseButtonUp(target.handle);
        }
    }

    REQUEST(AddButton) {
        auto handle = gui.add(Button(), gui.root());
        co_return handle;
    }

    TEMPLATE_REQUEST(AddGuiElement<T>, typename T) {
        auto handle = gui.add(request.element, request.parent.value_or(Gui::convertHandle(gui.root())));
        co_return handle;
    }

    REQUEST(GetGui) {
        co_return gui;
    }

private:
    Gui gui;
};

}
