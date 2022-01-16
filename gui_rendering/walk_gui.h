#pragma once

#include "gui_rendering/primitives.h"
#include "gui/gui.h"
#include <glm/glm.hpp>
#include <optional>

namespace pt {

class GuiVisitor {
    struct WalkContext {
        glm::uvec2 offset;
        std::optional<glm::mat4> view;
    };
public:
    GuiVisitor(VertexBufferBuilder& vbBuilder): vbBuilder(&vbBuilder) {}
    void operator()(const Gui& gui, const GuiHandle<Button>& handle, const WalkContext& c);
    void operator()(const Gui& gui, const GuiHandle<GuiRoot>& handle, const WalkContext& c);
    WalkContext operator()(const Gui& gui, const GuiHandle<Translate>& handle, WalkContext c);

    WalkContext operator()(const Gui& gui, const GuiHandle<MeshSet>& handle, WalkContext c);
    void operator()(const Gui& gui, const GuiHandle<GuiMesh>& handle, const WalkContext& c);

    void visit(const Gui& gui) {
        gui.visitAllWithContext(*this, WalkContext{});
    }
private:
    VertexBufferBuilder* vbBuilder;
};

}
