#include "framework/context.h"
#include "core_messages/control.h"
#include "window/window.h"
#include "rendering/vulkan.h"
#include "rendering/framerate_driver.h"

#include "application/triangle.h"

#include <future>

using namespace pt;

struct Quitter {
    EVENT(QuitRequested) {
        if constexpr (ctx.template can_handle<ProgramEnd>()) {
            co_await ctx.emit_await(pe);
        } 
        release_main.set_value(event.exit_status);
        co_return;
    }

    ProgramEnd pe;
    std::promise<int> release_main;
};

int main() {
    std::promise<int> release_main;
    std::future<int> release_main_future = release_main.get_future();

    auto context = Context{
        Quitter{ProgramEnd{}, std::move(release_main)},
        Window(800, 600, "Application"),
        VulkanRendering(2),
        FramerateDriver(60),
        Triangle(1.0),
    };

    if constexpr (context.can_handle<ProgramStart>()) {
        context.emit_sync(ProgramStart{});
    }

    return release_main_future.get();
}