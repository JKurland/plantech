#pragma once

#include "gui/element.h"
#include "utils/empty.h"
#include <glm/glm.hpp>

#include <cstdint>

namespace pt {

// Causes its children to be translated on screen by offset pixels
class Translate: public GuiElement {
public:
    explicit Translate(glm::uvec2 offset): offset(offset) {}
    Translate(unsigned x, unsigned y): offset(x, y) {}

    using PosDataT = Empty;
    glm::uvec2 offset;
};

}
