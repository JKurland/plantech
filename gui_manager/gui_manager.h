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

template<typename ElemT, typename = void>
struct GuiElementMouseButton {};

template<typename ElemT>
struct GuiElementMouseButton<ElemT, std::enable_if_t<std::is_void_v<typename ElemT::PosDataT>>> {
    MouseButton mouseEvent;
    GuiHandle<ElemT> element;
};

template<typename ElemT>
struct GuiElementMouseButton<ElemT, std::enable_if_t<!std::is_void_v<typename ElemT::PosDataT>>> {
    MouseButton mouseEvent;
    GuiHandle<ElemT> element;
    typename ElemT::PosDataT posData;
};

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
