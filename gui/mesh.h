#pragma once

#include "gui/element.h"
#include "utils/empty.h"
#include <glm/glm.hpp>
#include <vector>

namespace pt {

struct GuiMesh: GuiElement {
    using PosDataT = Empty;
    GuiMesh(std::vector<glm::vec3> points): points(std::move(points)) {}

    std::vector<glm::vec3> points;
};

struct MeshSet: GuiElement {
    using PosDataT = Empty;
    MeshSet(const glm::mat4& view): view(view) {}
    glm::mat4 view;
};

}
