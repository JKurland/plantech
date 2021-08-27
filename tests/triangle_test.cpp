#include <gtest/gtest.h>
#include "core_messages/control.h"
#include "framework/context.h"
#include "rendering/vulkan.h"
#include "window/window.h"
#include "tests/triangle.h"

#include <future>

using namespace pt;

class TriangleTest: public ::testing::Test {
protected:
    TriangleTest():
        p(),
        f(p.get_future()),
        context(make_context(
            Quitter{ProgramEnd{}, std::move(p)},
            Window(800, 600, "Triangle Test"),
            ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
            Triangle(1.0)
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
    }
private:
    std::promise<int> p;
    std::future<int> f;
    Context<
        Quitter,
        Window,
        VulkanRendering,
        Triangle
    > context;

};

TEST_F(TriangleTest, should_not_crash_if_destroyed_before_start) {}

TEST_F(TriangleTest, should_open_and_close_without_drawing_frame) {
    startProgram();
    quitProgram();
}

TEST_F(TriangleTest, should_open_and_close_window_and_draw_frame) {
    startProgram();
    newFrame();
    quitProgram();
}

TEST_F(TriangleTest, should_open_and_close_window_and_draw_some_frames) {
    startProgram();
    for (size_t i = 0; i < 10; i++) {
        newFrame();
    }
    quitProgram();
}
