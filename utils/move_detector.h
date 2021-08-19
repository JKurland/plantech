#pragma once

namespace pt {
struct MoveDetector {
    MoveDetector() = default;
    MoveDetector(const MoveDetector&) = default;
    MoveDetector(MoveDetector&& o): moved(o.moved) {o.moved = true;}

    MoveDetector& operator=(const MoveDetector&) = default;
    MoveDetector& operator=(MoveDetector&& o) {
        if (this == &o) return *this;
        moved = o.moved;
        o.moved = true;
        return *this;
    }

    bool moved = false;
};
}