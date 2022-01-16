#include "gui_rendering/walk_gui.h"
#include <memory>

namespace pt {

void GuiVisitor::operator()(const Gui& gui, const GuiHandle<Button>& handle, const WalkContext& c) {
    const auto& button = gui.get(handle);
    glm::vec3 colour; 
    if (button.clicked) {
        colour = glm::vec3(0.4, 0.4, 0.4);
    } else {
        colour = glm::vec3(0.5, 0.5, 0.5);
    }
    vbBuilder->addRectangle(
        glm::uvec2(0, 0) + c.offset,
        glm::uvec2(100, 40) + c.offset,
        0.1,
        colour,
        vbBuilder->addEventTarget(handle)
    );
}

void GuiVisitor::operator()(const Gui&, const GuiHandle<GuiRoot>& handle, const WalkContext& c) {
    // make the root node a black rectangle that covers the whole screen for now, this is so that it mops up any
    // unclaimed mouse events
    vbBuilder->addEventTarget(handle);
    vbBuilder->addBackground(0.9, glm::vec3{0.0, 0.0, 0.0}, vbBuilder->addEventTarget(handle));
}

GuiVisitor::WalkContext GuiVisitor::operator()(const Gui& gui, const GuiHandle<Translate>& handle, WalkContext c) {
    c.offset += gui.get(handle).offset;
    return c;
}


GuiVisitor::WalkContext GuiVisitor::operator()(const Gui& gui, const GuiHandle<MeshSet>& handle, WalkContext c) {
    c.view = gui.get(handle).view;
    return c;
}


void GuiVisitor::operator()(const Gui& gui, const GuiHandle<GuiMesh>& handle, const WalkContext& c) {
    auto target = vbBuilder->addEventTarget(handle);
    vbBuilder->addTriangles(gui.get(handle).points, glm::vec3(0.2, 0.2, 0.2), c.view.value_or(glm::mat4(1)), target);
}

}
