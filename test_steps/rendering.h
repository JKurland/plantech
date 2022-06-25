#pragma once

#include "test_utils/test.h"
#include "framework/context.h"
#include "rendering/framerate_driver.h"
#include "rendering/vulkan.h"

namespace pt::testing {

template<>
struct HandlerCreator<FramerateDriver, void> {
    auto makeContextArgs() {
        return FramerateDriver{30};
    }
};

template<>
struct HandlerCreator<VulkanRendering, void> {
    auto makeContextArgs() {
        return ctor_args<VulkanRendering>(2);
    }
};


struct WhenFramesDrawn: TestStep<RequiresNameOnly<"Context">> {
    void step(auto world) {
        auto& context = world.template getEntryByName<"Context">();
        for (size_t i = 0; i < 10; i++) {
            context.emit_sync(NewFrame{});
        }
    }
};

}

