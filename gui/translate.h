#pragma once

#include "gui/element.h"
#include <glm/glm.hpp>

namespace pt {

class Translate: public GuiElement {
public:
    using PosDataT = void;
    glm::vec2 offset;
};

}
