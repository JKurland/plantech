#include <gtest/gtest.h>
#include "core_messages/control.h"
#include "framework/context.h"
#include "rendering/vulkan.h"
#include "window/window.h"
#include "gui_rendering/gui_renderer.h"
#include "gui/gui.h"
#include "test_utils/fixture.h"

#include <future>
#include <functional>
#include <iostream>

using namespace pt;


struct MockGuiManager {
    REQUEST(GetGui) {
        co_return *gui;
    }

    Gui* gui;
};

class TestGuiRenderer: public ProgramControlFixture<TestGuiRenderer> {
protected:
    TestGuiRenderer():
        ProgramControlFixture(),
        context(make_context(
            takeQuitter(),
            Window(800, 600, "Triangle Test", pollWindow),
            ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
            ctor_args<GuiRenderer>(1.0),
            MockGuiManager{&gui}
        ))
    {}

    void newFrame() {
        context.emit_sync(NewFrame{});
    }

    auto addButton() {
        return gui.add(Button(), gui.root());
    }

    friend class ProgramControlFixture<TestGuiRenderer>;
private:
    // we don't call it in these tests but Window needs it to be happy
    std::function<void()> pollWindow;
    Gui gui;
    Context<
        Quitter,
        Window,
        VulkanRendering,
        GuiRenderer,
        MockGuiManager
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

    newFrame();
    quitProgram();
}

TEST_F(TestGuiRenderer, should_open_and_close_window_and_draw_some_frames) {
    startProgram();
    addButton();
    for (size_t i = 0; i < 10; i++) {
        newFrame();
    }
    quitProgram();
}

TEST_F(TestGuiRenderer, should_open_and_close_window_and_draw_frame_with_2_buttons) {
    startProgram();

    addButton();
    addButton();

    newFrame();
    quitProgram();
}
