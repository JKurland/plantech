#include "gui/gui.h"

namespace pt {

Gui::Gui(): next_handle_index(1) {
    nodes.emplace(
        0,
        Node{
            std::make_unique<GuiRoot>(),
            GuiHandle(0),
            std::vector<GuiHandle>{}
        }
    );
}

GuiHandle Gui::add(std::unique_ptr<GuiElement> element, GuiHandle parent) {
    nodes.emplace(
        next_handle_index,
        Node{
            std::move(element),
            parent,
            std::vector<GuiHandle>{} // children
        }
    );

    GuiHandle new_handle(next_handle_index);
    next_handle_index++;

    auto parent_iter = nodes.find(parent.index);
    parent_iter->second.children.push_back(new_handle);
    return new_handle;
}

GuiHandle Gui::root() {
    return GuiHandle(0);
}


void Gui::mouseButtonDown(GuiHandle target) {
    get(target).mouseButtonDown();
    mouseFocusedNode = target.index;
}

void Gui::mouseButtonUp(GuiHandle target) {
    get(target).mouseButtonUp();

    if (mouseFocusedNode == 0) return;

    auto ret = nodes.find(mouseFocusedNode);
    if (ret != nodes.end()) {
        ret->second.element->mouseButtonUp();
    }

    mouseFocusedNode = 0;
}

}