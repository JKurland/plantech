#include "framework/context.h"
#include "core_messages/control.h"
#include "window/window.h"
#include "rendering/vulkan.h"
#include "rendering/framerate_driver.h"
#include "rendering/mesh.h"
#include "application/click_for_shape.h"
#include "gui_rendering/gui_renderer.h"
#include "gui_manager/gui_manager.h"

#include <future>
#include <functional>

using namespace pt;



int main() {
    // filled in by Window and called from main 
    // since glfw requires many functions to be
    // called on the main thread.
    std::function<void()> pollWindow;

    std::promise<int> releaseMain;
    std::future<int> releaseMainFuture = releaseMain.get_future();

    /* context is created as follows:
        Each handler is added to the context in the order they appear in the arguments to make_context.
        If the argument to make context is a CtorArgs then instead of being interpreted as a handler the constructor
          of the CtorArgs template argument is called. The first argument passed to the constructor is the context
          containing all the handlers added up to that point, the remaining arguments are forwarded from the arguments to
          the CtorArgs constructor.
        
        Doing this allows each handler to use context inside it's constructor, it can communicate with the other handlers that
        have already been constructed, and it is checked at compile time if it tries to issue requests to handlers that are not
        constructed yet.

        This does mean that the context supplied during construction is not the same as the context supplied in handlers, so don't
        save off the constructor context, wait for ProgramStart instead.

        When context is destructed destructors are run in the opposite order to constructors. Handlers won't receive events or requests
        during destruction, this is because handlers mustn't send events or requests after returning from their EVENT(ProgramEnd).
    */
    auto context = make_context(
        Quitter(ProgramEnd{}, std::move(releaseMain)),
        Window(800, 600, "Application", pollWindow),
        ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
        FramerateDriver(/*fps*/ 60),
        ctor_args<GuiRenderer>(),
        GuiManager{}
    );

    context.emit_sync(ProgramStart{});

    // pollWindow will only return when the program ends
    pollWindow();

    int ret = releaseMainFuture.get();
    context.no_more_messages();
    context.wait_for_all_events_to_finish();
    return ret;
}
