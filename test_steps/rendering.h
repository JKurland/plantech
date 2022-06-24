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

}

