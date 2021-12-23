#include <gtest/gtest.h>

#include "gui/gui.h"
#include "gui_rendering/gui_renderer.h"
#include "framework/context.h"
#include "test_utils/fixture.h"
#include "gui_manager/gui_manager.h"

using namespace pt;

class TestTranslate: public ProgramControlFixture<TestTranslate> {
protected:
    TestTranslate():
        ProgramControlFixture(),
        context(make_context(
            takeQuitter(),
            Window(800, 600, "Translate Test", pollWindow),
            ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
            ctor_args<GuiRenderer>(),
            GuiManager{}
        ))
    {}

    GuiHandle<Button> addButton(int x, int y) {
        GuiHandle<Translate> translateHandle = context.request_sync(AddGuiElement<Translate>{
            .parent = std::nullopt,
            .element = Translate(x, y),
        });

        GuiHandle<Button> button = context.request_sync(AddGuiElement<Button>{
            .parent = Gui::convertHandle(translateHandle),
            .element = Button(),
        });

        newFrame();

        return button;
    }

        void assertClicked(GuiHandle<Button> handle) {
        ASSERT_TRUE(context.request_sync(GetGui{}).get(handle).clicked);
    }

    void assertUnclicked(GuiHandle<Button> handle) {
        ASSERT_FALSE(context.request_sync(GetGui{}).get(handle).clicked);
    }

    void mouseDown(double x, double y) {
        context.emit_sync(MouseButton{
            GLFW_MOUSE_BUTTON_LEFT,
            GLFW_PRESS,
            0,
            x,
            y,
            x/800.0,
            y/600.0
        });
    }

    void newFrame() {
        context.emit_sync(NewFrame{});
    }

    friend class ProgramControlFixture<TestTranslate>;

private:
    std::function<void()> pollWindow;

    Context<
        Quitter,
        Window,
        VulkanRendering,
        GuiRenderer,
        GuiManager
    > context;
};


TEST_F(TestTranslate, test_button_click_on_translated_button) {
    startProgram();

    auto button = addButton(300, 300);
    assertUnclicked(button);
    mouseDown(310, 310);
    assertClicked(button);

    quitProgram();
}

TEST_F(TestTranslate, test_button_click_not_on_translated_button) {
    startProgram();

    auto button = addButton(300, 300);
    assertUnclicked(button);
    mouseDown(100, 100);
    assertUnclicked(button);

    quitProgram();
}
