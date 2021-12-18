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
            FramerateDriver{30},
            ctor_args<GuiRenderer>(1.0),
            GuiManager{}
        ))
    {}

    GuiHandle<Button> addButton() {
        return context.request_sync(AddButton{});
    }

    void poll() {
        pollWindow();
    }

    friend class ProgramControlFixture<TestGuiManager>;
private:
    std::function<void()> pollWindow;

    Context<
        Quitter,
        Window,
        VulkanRendering,
        FramerateDriver,
        GuiRenderer,
        GuiManager
    > context;
};

TEST_F(TestGuiManager, keep_alive_with_button) {
    startProgram();
    addButton();
    poll();
}
