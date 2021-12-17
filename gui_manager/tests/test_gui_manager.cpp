#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "gui_manager/gui_manager.h"
#include "rendering/framerate_driver.h"

using namespace pt;

class TestGuiManager: public ::testing::Test {
protected:
    TestGuiManager():
        p(),
        f(p.get_future()),
        context(make_context(
            Quitter{ProgramEnd{}, std::move(p)},
            Window(800, 600, "Triangle Test", pollWindow),
            ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
            FramerateDriver{30},
            ctor_args<GuiRenderer>(1.0),
            GuiManager{}
        ))
    {}

    void startProgram() {
        context.emit_sync(ProgramStart{});
    }

    void quitProgram() {
        context.emit_sync(QuitRequested{0});
        ASSERT_EQ(f.get(), 0);
        context.no_more_messages();
        context.wait_for_all_events_to_finish();
    }

    GuiHandle<Button> addButton() {
        return context.request_sync(AddButton{});
    }

    void poll() {
        pollWindow();
    }

private:
    std::promise<int> p;
    std::future<int> f;

    // we don't call it in these tests but Window needs it to be happy
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
