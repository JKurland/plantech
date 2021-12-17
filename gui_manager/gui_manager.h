#pragma once

#include "framework/context.h"
#include "gui_rendering/gui_renderer.h"
#include "window/window.h"
#include "gui/button.h"

namespace pt {

struct AddButton {
    using ResponseT = GuiHandle<Button>;
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

    REQUEST(GetGui) {
        co_return gui;
    }

private:
    Gui gui;
};

}
