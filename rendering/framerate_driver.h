#pragma once

#include "core_messages/control.h"
#include "framework/context.h"
#include "thread_pool/sleep.h"
#include "rendering/vulkan.h"
#include "window/window.h"

namespace pt {

namespace framerate_driver::detail {
    struct StartDriverLoop {};
}

class FramerateDriver {
public:
    FramerateDriver(double target_fps): target_fps(target_fps) {}

    EVENT(ProgramStart) {
        ctx.emit(framerate_driver::detail::StartDriverLoop{});
        co_return;
    }

    EVENT(ProgramEnd) {
        stop = true;
        co_return;
    }

    EVENT(framerate_driver::detail::StartDriverLoop) {
        while (!stop) {
            auto next_frame = 
                std::chrono::steady_clock::now() + 
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(1/target_fps));

            if (!pause) {
                co_await ctx.emit_await(NewFrame{});
            }
            co_await sleep_until(next_frame);
        }        
    }

    EVENT(WindowResize) {
        if (event.width == 0 && event.height == 0) {
            pause = true;
        } else {
            pause = false;
        }
        co_return;
    }

    EVENT(WindowMinimised) {
        pause = true;
        co_return;
    }

    EVENT(WindowRestored) {
        pause = false;
        co_return;
    }

private:
    bool stop = false;
    bool pause = false;
    double target_fps;
};

}
