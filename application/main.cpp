#include "framework/context.h"
#include "core_messages/control.h"
#include "window/window.h"
#include "rendering/vulkan.h"
#include "rendering/framerate_driver.h"

#include <future>

using namespace pt;



int main() {
    std::promise<int> release_main;
    std::future<int> release_main_future = release_main.get_future();

    auto context = Context{
        Quitter{ProgramEnd{}, std::move(release_main)},
        Window(800, 600, "Application"),
        VulkanRendering(/*max frames in flight*/ 2),
        FramerateDriver(/*fps*/ 60),
    };

    context.emit_sync(ProgramStart{});

    return release_main_future.get();
}
