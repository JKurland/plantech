#pragma once

#include "gui/element.h"

namespace pt {

class Button: public GuiElement {
public:
    bool clicked;

    void mouseButtonDown() override {clicked = true;}
    void mouseButtonUp() override {clicked = false;}
};

}
