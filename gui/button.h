#pragma once

#include "gui/element.h"

namespace pt {

class Button: public GuiElement {
public:
    // is the button in the process of being clicked. So button down but not yet button up.
    bool clicked;

    void mouseButtonDown() override {clicked = true;}
    void mouseButtonUp() override {clicked = false;}
};

}
