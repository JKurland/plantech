#include "framework/context.h"
#include "core_messages/control.h"
#include "window/window.h"
#include "rendering/vulkan.h"
#include "rendering/framerate_driver.h"
#include "rendering/mesh.h"

#include <future>

using namespace pt;



int main() {
    std::promise<int> release_main;
    std::future<int> release_main_future = release_main.get_future();

    /* context is created as follows:
        Each handler is added to the context in the order they appear in the arguments to make_context.
        If the argument to make context is a CtorArgs then instead of being interpreted as a handler the constructor
          of the CtorArgs template argument is called. The first argument passed to the constructor is the context
          containing all the handlers add up to that point, the remaining arguments are forwarded from the arguments to
          the CtorArgs constructor.
        
        Doing this allows each handler to use context inside it's constructor, it can communicate with the other handlers that
        have already been constructed, and it is checked at compile time if it tries to issue requests to handlers that are not
        constructed yet.

        This does mean the the context supplied during construction is not the same as the context supplied in handlers, so don't
        save off the constructor context.

        When context is destructed destructors are run in the opposite order to constructors. Handlers won't receive events or requests
        during destruction, this is because handlers mustn't send events or requests after returning from their EVENT(ProgramEnd).
    */
    auto context = make_context(
        Quitter(ProgramEnd{}, std::move(release_main)),
        Window(800, 600, "Application"),
        ctor_args<VulkanRendering>(/*max frames in flight*/ 2),
        FramerateDriver(/*fps*/ 60),
        ctor_args<MeshRenderer>()
    );

    context.emit_sync(ProgramStart{});

    return release_main_future.get();
}
