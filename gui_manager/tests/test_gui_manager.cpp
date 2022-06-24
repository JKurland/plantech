#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "gui_manager/gui_manager.h"
#include "rendering/framerate_driver.h"
#include "test_utils/fixture.h"


using namespace pt;

// x and y coords that are outside the button
constexpr double outsideButton = 200;

class TestGuiManager: public ProgramControlFixture<TestGuiManager> {
protected:
    TestGuiManager():
        ProgramControlFixture(),
        context(make_context(
            takeQuitter(),
            Window(800, 600, "Triangle Test", pollWindow),
            ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
            ctor_args<GuiRenderer>(),
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
        });
    }

    void buttonUp(double x, double y) {
        context.emit_sync(MouseButton{
            GLFW_MOUSE_BUTTON_LEFT,
            GLFW_RELEASE,
            0,
            x,
            y,
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

TEST_F(TestGuiManager, button_detects_a_click_and_far_away_unclick) {
    startProgram();

    auto button = addButton();
    newFrame();

    buttonDown(1, 1);
    assertClicked(button);
    buttonUp(outsideButton, outsideButton);
    assertUnclicked(button);

    quitProgram();
}

TEST_F(TestGuiManager, button_rejects_a_non_click) {
    startProgram();

    auto button = addButton();
    newFrame();

    buttonDown(outsideButton, outsideButton);
    assertUnclicked(button);

    quitProgram();
}
