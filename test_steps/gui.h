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
    int width;
    int height;
    GuiHandle<Button> handle;
    GuiHandle<Translate> translateHandle;
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

        return this->worldUpdate(
            WorldEntry<Name, ButtonInfo>{ButtonInfo{100, 40, button, translateHandle}}
        );
    }
};

// only works for 2 buttons, subsequent instances of this step can overwrite this step.
template<WorldEntryName Name1, WorldEntryName Name2>
struct GivenButtonsDoNotOverlap: TestStep<RequiresNameOnly<"Context">, Requires<Name1, ButtonInfo>, Requires<Name2, ButtonInfo>> {
    void step(auto world) {
        auto& context = world.template getEntryByName<"Context">();
        ButtonInfo& button1 = world.template getEntry<Name1, ButtonInfo>();
        ButtonInfo button2 = world.template getEntry<Name2, ButtonInfo>();

        Translate newTranslate = *context.request_sync(GetGuiElement<Translate>{button1.translateHandle});
        Translate translate2 = *context.request_sync(GetGuiElement<Translate>{button2.translateHandle});
        newTranslate.offset.x = translate2.offset.x + button2.width + 2;
        newTranslate.offset.y = translate2.offset.y + button2.height + 2;

        context.request_sync(UpdateGuiElement{button1.translateHandle, newTranslate});

    }
};

template<WorldEntryName Name>
struct WhenButtonClicked: TestStep<Requires<Name, ButtonInfo>, RequiresNameOnly<"Context">> {
    void step(auto world) {
        auto& context = world.template getEntryByName<"Context">();
        const ButtonInfo& buttonInfo = world.template getEntry<Name, ButtonInfo>();
        Translate translate = *context.request_sync(GetGuiElement<Translate>(buttonInfo.translateHandle));

        double clickX = translate.offset.x + buttonInfo.width/2;
        double clickY = translate.offset.y + buttonInfo.height/2;

        context.emit_sync(NewFrame{});
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
        Translate translate = *context.request_sync(GetGuiElement<Translate>(buttonInfo.translateHandle));

        double clickX = translate.offset.x + buttonInfo.width + 2;
        double clickY = translate.offset.y + buttonInfo.height + 2;

        context.emit_sync(NewFrame{});
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
