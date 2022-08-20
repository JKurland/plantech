#pragma once

#include "gui/element.h"
#include "glm/glm.hpp"

namespace pt {

class ProjectMeshes: public GuiElement {
public:

private:
    glm::mat4 viewTransform;
    glm::mat4 projectionTransform;
};

}