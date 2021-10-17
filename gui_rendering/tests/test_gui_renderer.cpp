#include <gtest/gtest.h>
#include "core_messages/control.h"
#include "framework/context.h"
#include "rendering/vulkan.h"
#include "window/window.h"
#include "gui_rendering/gui_renderer.h"
#include "gui/gui.h"

#include <future>
#include <functional>

using namespace pt;

class TestGuiRenderer: public ::testing::Test {
protected:
    TestGuiRenderer():
        p(),
        f(p.get_future()),
        context(make_context(
            Quitter{ProgramEnd{}, std::move(p)},
            Window(800, 600, "Triangle Test", pollWindow),
            ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
            ctor_args<GuiRenderer>(1.0)
        ))
    {}

    void startProgram() {
        context.emit_sync(ProgramStart{});
    }

    void newFrame() {
        context.emit_sync(NewFrame{});
    }

    void quitProgram() {
        context.emit_sync(QuitRequested{0});
        ASSERT_EQ(f.get(), 0);
        context.no_more_messages();
        context.wait_for_all_events_to_finish();
    }

    auto addButton() {
        return gui.add(Button(), gui.root());
    }

    void updateGui() {
        context.emit_sync(GuiUpdated{.newGui = gui});
    }
private:
    std::promise<int> p;
    std::future<int> f;

    // we don't call it in these tests but Window needs it to be happy
    std::function<void()> pollWindow;
    Gui gui;
    Context<
        Quitter,
        Window,
        VulkanRendering,
        GuiRenderer
    > context;
};


TEST_F(TestGuiRenderer, should_not_crash_if_destroyed_before_start) {}

TEST_F(TestGuiRenderer, should_open_and_close_without_drawing_frame) {
    startProgram();
    quitProgram();
}

TEST_F(TestGuiRenderer, should_open_and_close_window_and_empty_draw_frame) {
    startProgram();
    newFrame();
    quitProgram();
}

TEST_F(TestGuiRenderer, should_open_and_close_window_and_draw_some_empty_frames) {
    startProgram();
    for (size_t i = 0; i < 10; i++) {
        newFrame();
    }
    quitProgram();
}

TEST_F(TestGuiRenderer, should_open_and_close_window_and_draw_frame_with_button) {
    startProgram();

    addButton();
    updateGui();

    newFrame();
    quitProgram();
}

TEST_F(TestGuiRenderer, should_open_and_close_window_and_draw_some_frames) {
    startProgram();
    addButton();
    updateGui();
    for (size_t i = 0; i < 10; i++) {
        newFrame();
    }
    quitProgram();
}

TEST_F(TestGuiRenderer, should_open_and_close_window_and_draw_frame_with_2_buttons) {
    startProgram();

    addButton();
    addButton();
    updateGui();

    newFrame();
    quitProgram();
}