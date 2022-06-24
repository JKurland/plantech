#include "test_steps/context.h"

#include "gui/gui.h"
#include "gui_rendering/gui_renderer.h"
#include "gui_manager/gui_manager.h"

#include <GLFW/glfw3.h>

namespace pt::testing {

template<>
struct HandlerCreator<GuiRenderer, void> {
    auto makeContextArgs() {
        return ctor_args<GuiRenderer>();
    }
};


struct ButtonInfo {
    int x;
    int y;
    int width;
    int height;
    GuiHandle<Button> handle;
};

template<WorldEntryName Name>
struct GivenAButton: TestStep<Provides<Name, ButtonInfo>, RequiresNameOnly<"Context">> {
    auto step(auto world) {
        auto& context = world.template getEntryByName<"Context">();

        int x = 300;
        int y = 300;

        GuiHandle<Translate> translateHandle = context.request_sync(AddGuiElement<Translate>{
            .parent = std::nullopt,
            .element = Translate(x, y),
        });

        GuiHandle<Button> button = context.request_sync(AddGuiElement<Button>{
            .parent = Gui::convertHandle(translateHandle),
            .element = Button(),
        });

        context.emit_sync(NewFrame{});

        return this->worldUpdate(
            WorldEntry<Name, ButtonInfo>{ButtonInfo{x, y, 100, 40, button}}
        );
    }
};

template<WorldEntryName Name>
struct WhenButtonClicked: TestStep<Requires<Name, ButtonInfo>, RequiresNameOnly<"Context">> {
    void step(auto world) {
        auto& context = world.template getEntryByName<"Context">();
        const ButtonInfo& buttonInfo = world.template getEntry<Name, ButtonInfo>();

        double clickX = buttonInfo.x + buttonInfo.width/2;
        double clickY = buttonInfo.y + buttonInfo.height/2;

        context.emit_sync(MouseButton{
            GLFW_MOUSE_BUTTON_LEFT,
            GLFW_PRESS,
            0,
            clickX,
            clickY,
        });
    }
};

template<WorldEntryName Name>
struct WhenClickNotOnButton: TestStep<Requires<Name, ButtonInfo>, RequiresNameOnly<"Context">> {
    void step(auto world) {
        auto& context = world.template getEntryByName<"Context">();
        const ButtonInfo& buttonInfo = world.template getEntry<Name, ButtonInfo>();

        double clickX = buttonInfo.x + buttonInfo.width + 2;
        double clickY = buttonInfo.y + buttonInfo.height + 2;

        context.emit_sync(MouseButton{
            GLFW_MOUSE_BUTTON_LEFT,
            GLFW_PRESS,
            0,
            clickX,
            clickY,
        });
    }
};


template<WorldEntryName Name>
struct ThenButtonClickedEventIsEmitted: TestStep<
    Requires<Name, ButtonInfo>,
    Requires<"SentMessages", SentMessages>
> {
    void step(auto world) {
        const ButtonInfo& buttonInfo = world.template getEntry<Name, ButtonInfo>();
        const SentMessages& sentMessages = world.template getEntry<"SentMessages", SentMessages>();

        const bool gotSent = sentMessages.template hasMessageMatchingPred<GuiElementMouseButton<Button>>(
            [&](const GuiElementMouseButton<Button>& message) {
                return message.element == buttonInfo.handle;
            }
        );

        if (!gotSent) {
            throw "Button click did not get sent";
        }
    }
};

}
