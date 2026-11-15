#pragma once

#include "math.h"
#include <stdexcept>

namespace kinetra {

/// Camera with Y-up world coordinates and Y-down screen pixels. Screen dimensions
/// are logical window pixels (not framebuffer pixels), so picking works at high DPI.
struct Camera2D {
    Vec2 position{};
    float pixels_per_unit{20};
    Vec2 viewport{1280, 800};
    void validate() const {
        if (!finite(position) || !finite(viewport) || viewport.x <= 0 || viewport.y <= 0 ||
            !std::isfinite(pixels_per_unit) || pixels_per_unit <= 0)
            throw std::invalid_argument("Invalid camera position, viewport or zoom");
    }
    Vec2 world_to_screen(Vec2 point) const {
        validate();
        const Vec2 delta = (point - position) * pixels_per_unit;
        return {viewport.x / 2 + delta.x, viewport.y / 2 - delta.y};
    }
    Vec2 screen_to_world(Vec2 point) const {
        validate();
        return position + Vec2{point.x - viewport.x / 2, viewport.y / 2 - point.y} / pixels_per_unit;
    }
};

} // namespace kinetra
