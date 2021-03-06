#pragma once

#include "gui/element.h"
#include "utils/empty.h"

namespace pt {

class Button: public GuiElement {
public:
    using PosDataT = Empty;

    // is the button in the process of being clicked. So button down but not yet button up.
    bool clicked = false;

    void mouseButtonDown() override {clicked = true;}
    void mouseButtonUp() override {clicked = false;}
};

}
