#include "gui_rendering/walk_gui.h"
#include <memory>

namespace pt {

void GuiVisitor::operator()(const Gui& gui, const GuiHandle<Button>& handle) {
    const auto& button = gui.get(handle);
    glm::vec3 colour; 
    if (button.clicked) {
        colour = glm::vec3(0.4, 0.4, 0.4);
    } else {
        colour = glm::vec3(0.5, 0.5, 0.5);
    }
    vbBuilder->addRectangle(
        glm::uvec2(0, 0),
        glm::uvec2(100, 40),
        0.1,
        colour,
        vbBuilder->addEventTarget(handle)
    );
}

void GuiVisitor::operator()(const Gui&, const GuiHandle<GuiRoot>& handle) {
    // make the root node a black rectangle that covers the whole screen for now
    vbBuilder->addEventTarget(handle);
    vbBuilder->addBackground(0.9, glm::vec3{0.0, 0.0, 0.0}, vbBuilder->addEventTarget(handle));
}

}
