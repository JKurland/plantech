#pragma once

#include "framework/context.h"
#include "gui_rendering/gui_renderer.h"
#include "window/window.h"
#include "gui/button.h"

#include <optional>

namespace pt {


class GuiManager {
public:
    EVENT(MouseButton) {
        auto req = GetEventTargetForPixel{static_cast<uint32_t>(event.x), static_cast<uint32_t>(event.y)};
        EventTarget target = co_await ctx(req);
        gui.visitHandle(target.handle, [&]<typename ElemT>(const GuiHandle<ElemT>& handle){
            using PosDataT = typename ElemT::PosDataT;

            auto toEmit = [&]{
                if constexpr (std::is_void_v<PosDataT>) {
                    return GuiElementMouseButton<ElemT>{
                        event,
                        handle,
                    };
                } else {
                    return GuiElementMouseButton<ElemT>{
                        event,
                        handle,
                        PosDataT(*static_cast<PosDataT*>(target.posData.get()))
                    };
                }
            }();

            ctx.emit(toEmit);
        });
    }

    TEMPLATE_EVENT(GuiElementMouseButton<T>, typename T) {
        if (event.mouseEvent.action == GLFW_PRESS) {
            gui.mouseButtonDown(gui.convertHandle(event.element));
        } else {
            gui.mouseButtonUp(gui.convertHandle(event.element));
        }
        co_return;
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
