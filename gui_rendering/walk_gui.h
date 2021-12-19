#pragma once

#include "gui_rendering/primitives.h"
#include "gui/gui.h"


namespace pt {

class GuiVisitor {
public:
    GuiVisitor(VertexBufferBuilder& vbBuilder): vbBuilder(&vbBuilder) {}
    void operator()(const Gui& gui, const GuiHandle<Button>& handle);
    void operator()(const Gui& gui, const GuiHandle<GuiRoot>& handle);
    void operator()(const Gui& gui, const GuiHandle<Translate>& handle);

    void visit(const Gui& gui) {
        gui.visitAll(*this);
    }
private:
    VertexBufferBuilder* vbBuilder;
};

}
