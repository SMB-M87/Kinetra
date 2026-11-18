#pragma once

#include "math.h"
#include <algorithm>
#include <cmath>
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

/// Inclusive camera zoom range in pixels per world unit; minimum <= maximum.
struct ZoomLimits {
    float minimum, maximum;
};

/// Throws std::invalid_argument unless 0 < minimum <= maximum and both are finite.
inline void validate(ZoomLimits limits) {
    if (!std::isfinite(limits.minimum) || !std::isfinite(limits.maximum) || limits.minimum <= 0 ||
        limits.minimum > limits.maximum)
        throw std::invalid_argument("Invalid zoom limits");
}

/// Zoom change per step: one wheel notch or one ZOOM IN / ZOOM OUT click.
inline constexpr float zoom_factor = 1.1f;

/// Zooms `camera` by `steps` steps of zoom_factor (positive zooms in) and clamps the result
/// to `limits`. Fractional steps come from smooth wheels. Zero steps still clamp, so a
/// change of limits (for example leaving a debug mode) takes effect on the next frame.
/// Throws std::invalid_argument when the limits are not finite, positive and ordered.
inline void zoom(Camera2D &camera, float steps, ZoomLimits limits) {
    validate(limits);
    camera.pixels_per_unit =
        std::clamp(camera.pixels_per_unit * std::pow(zoom_factor, steps), limits.minimum, limits.maximum);
}

} // namespace kinetra
