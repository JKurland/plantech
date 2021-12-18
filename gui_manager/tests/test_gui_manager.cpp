#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "gui_manager/gui_manager.h"
#include "rendering/framerate_driver.h"
#include "test_utils/fixture.h"

using namespace pt;

class TestGuiManager: public ProgramControlFixture<TestGuiManager> {
protected:
    TestGuiManager():
        ProgramControlFixture(),
        context(make_context(
            takeQuitter(),
            Window(800, 600, "Triangle Test", pollWindow),
            ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
            ctor_args<GuiRenderer>(1.0),
            GuiManager{}
        ))
    {}

    GuiHandle<Button> addButton() {
        return context.request_sync(AddButton{});
    }

    void assertClicked(GuiHandle<Button> handle) {
        ASSERT_TRUE(context.request_sync(GetGui{}).get(handle).clicked);
    }

    void assertUnclicked(GuiHandle<Button> handle) {
        ASSERT_FALSE(context.request_sync(GetGui{}).get(handle).clicked);
    }

    void buttonDown(double x, double y) {
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

    void buttonUp(double x, double y) {
        context.emit_sync(MouseButton{
            GLFW_MOUSE_BUTTON_LEFT,
            GLFW_RELEASE,
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

    friend class ProgramControlFixture<TestGuiManager>;
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

TEST_F(TestGuiManager, button_detects_a_click) {
    startProgram();

    auto button = addButton();
    newFrame();

    buttonDown(1, 1);
    assertClicked(button);
    buttonUp(1, 1);
    assertUnclicked(button);

    quitProgram();
}


TEST_F(TestGuiManager, button_rejects_a_non_click) {
    startProgram();

    auto button = addButton();
    newFrame();

    buttonDown(200, 200);
    assertUnclicked(button);

    quitProgram();
}
