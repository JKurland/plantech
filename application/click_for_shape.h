#pragma once

#include "framework/context.h"
#include "rendering/mesh.h"
#include "window/window.h"

#include <iostream>

namespace pt {

class ClickForShape {
public:
    EVENT(MouseButton) {
        std::cout << event.x << ", " << event.y << std::endl;
        co_return;
    }
private:
};

}
