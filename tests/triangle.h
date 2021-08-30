#pragma once


#include "rendering/mesh.h"
#include "framework/context.h"

#include <vector>

namespace pt {

class Triangle {
public:
    EVENT(ProgramStart) {
        auto req = AddMesh{Mesh{{
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        }}};
        co_await ctx(req);
    }
};

}
