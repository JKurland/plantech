#pragma once

#include "window/window.h"
#include "test_steps/context.h"


#include <functional>

namespace pt::testing {

template<>
class HandlerCreator<Window, void> {
public:
    auto makeContextArgs() {
        return Window(800, 600, "Test", pollWindow);
    }
private:
    std::function<void()> pollWindow;
};

}
