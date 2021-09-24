#pragma once

namespace pt {

struct GuiElement {
    virtual ~GuiElement() = default;

    virtual void mouseButtonDown() {}
    virtual void mouseButtonUp() {}
};

}
