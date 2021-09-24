#pragma once

#include <unordered_map>
#include <typeindex>
#include <memory>
#include <vector>

#include "gui/element.h"

namespace pt {


class GuiRoot: public GuiElement {};

class GuiHandle {
    GuiHandle(size_t index): index(index) {}
    size_t index;
    friend class Gui;
};


class Gui {
public:
    Gui();
    GuiHandle add(std::unique_ptr<GuiElement> element, GuiHandle parent);

    GuiHandle root();

    template<typename T=GuiElement>
    T& get(GuiHandle handle) {return *dynamic_cast<T*>(nodes.find(handle.index)->second.element.get());}

    template<typename T=GuiElement>
    const T& get(GuiHandle handle) const {return *dynamic_cast<T*>(nodes.find(handle.index)->second.element.get());}

    void mouseButtonDown(GuiHandle target);
    void mouseButtonUp(GuiHandle target);
private:
    size_t next_handle_index;

    struct Node {
        std::unique_ptr<GuiElement> element;
        GuiElement* parent;
        std::vector<GuiElement*> children;
    };

    std::unordered_map<size_t, Node> nodes;

    size_t mouseFocusedNode = 0;
};

}
