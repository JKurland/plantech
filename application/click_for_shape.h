#pragma once

#include "framework/context.h"
#include "rendering/mesh.h"
#include "window/window.h"

#include <iostream>

namespace pt {

class ClickForShape {
public:
    EVENT(MouseButton) {
        std::cout << event.x_rel << ", " << event.y_rel << std::endl;
        double x_rel = event.x_rel;
        double y_rel = event.y_rel;

        auto req = AddMesh{Mesh{{
            {{x_rel, y_rel-0.02}, {1.0f, 0.0f, 0.0f}},
            {{x_rel+0.1, y_rel+0.01}, {0.0f, 1.0f, 0.0f}},
            {{x_rel-0.1, y_rel+0.01}, {0.0f, 0.0f, 1.0f}},
        }}};
        co_await ctx(req);
        co_return;
    }
};

}
