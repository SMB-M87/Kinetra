#pragma once

#include "bitmap_text.h"
#include <functional>
#include <span>
#include <string_view>

namespace kinetra {
/// One row of the debug menu. Set `enabled` for a check box or `action` for a button; the
/// other fields add read-only content to the row.
struct DebugOption {
    /// Row text; the viewed characters must outlive the menu's update() and draw() calls.
    std::string_view label;
    /// Check box state, toggled by a click. Borrowed; must outlive update/draw.
    bool *enabled{};
    /// Called on a click when `enabled` is null.
    std::function<void()> action{};
    /// Optional text appended to the label on every draw.
    std::function<std::string()> text{};
    /// Optional number appended to the label on every draw.
    std::function<double()> number{};
    /// Optional custom drawing next to the row: runtime, top-left screen position of the
    /// row text, and the menu scale.
    std::function<void(Runtime &, Vec2, float)> visualization{};
};

/// Shared screen-space debug controls. Layout is identical for drawing and picking;
/// all pointer input inside the panel is consumed, including held fire.
///
/// Positions are logical window pixels. The panel is 182 panel pixels wide, with a 22-pixel
/// heading and 16 pixels per option, scaled by scale().
struct DebugMenu {
    /// Whether the panel shows.
    bool visible{};
    /// Last pointer position, used for hover highlighting in draw().
    Vec2 pointer{-1, -1};
    /// Negative x preserves the default right-aligned layout. Otherwise use fixed screen pixels.
    Vec2 screen_origin{-1, -1};
    /// Top-left corner of the panel for `viewport`: `screen_origin`, or near the top-right
    /// corner by default.
    Vec2 origin(Vec2 viewport) const {
        const float size = scale(viewport);
        return screen_origin.x < 0 ? Vec2{viewport.x - 190 * size, 8 * size} : screen_origin;
    }
    /// Screen pixels per panel pixel for a viewport, at most 1.5.
    static float scale(Vec2 viewport) { return std::min({1.5f, viewport.x / 300.f, viewport.y / 260.f}); }
    /// Applies one frame of input: `toggle` shows or hides the panel, and a click on a row
    /// toggles its flag or runs its action. Returns true when the pointer is over the
    /// visible panel, so the caller must ignore that pointer input.
    bool update(const Input &input, Vec2 viewport, std::span<DebugOption> options,
                Key toggle = Key::backquote) {
        pointer = input.mouse;
        if (input.hit(toggle))
            visible = !visible;
        const float size = scale(viewport);
        const Vec2 point = (input.mouse - origin(viewport)) / size;
        const bool captured = visible && point.x >= 0 && point.x <= 182 && point.y >= 0 &&
                              point.y < static_cast<float>(22 + options.size() * 16);
        if (captured && input.clicked && point.y >= 22) {
            const auto row = std::size_t((point.y - 22) / 16);
            if (options[row].enabled)
                *options[row].enabled = !*options[row].enabled;
            else if (options[row].action)
                options[row].action();
        }
        return captured;
    }
    /// Draws the panel in screen space when visible; pass the same options as to update().
    void draw(Runtime &runtime, std::span<const DebugOption> options) const {
        if (!visible)
            return;
        const float size = scale(runtime.camera.viewport);
        const Vec2 origin = this->origin(runtime.camera.viewport);
        const Vec2 half{91 * size, static_cast<float>(22 + options.size() * 16) * size / 2};
        runtime.box(runtime.camera.screen_to_world(origin + half), half / runtime.camera.pixels_per_unit, 0,
                    {0.04f, 0.07f, 0.12f});
        draw_text(runtime, "DEBUG MENU", origin + Vec2{4, 6} * size, size, {1, 1, 1});
        const Vec2 point = (pointer - origin) / size;
        for (std::size_t row = 0; row < options.size(); ++row) {
            const auto &option = options[row];
            std::string text = std::string(option.enabled ? (*option.enabled ? "[X] " : "[ ] ") : "") +
                               std::string(option.label);
            if (option.text)
                text += " " + option.text();
            if (option.number)
                text += " " + std::to_string(option.number());
            const bool hovered = (option.enabled || option.action) && point.x >= 0 && point.x <= 182 &&
                                 point.y >= 22 + static_cast<float>(row) * 16.f &&
                                 point.y < 38 + static_cast<float>(row) * 16.f;
            draw_text(runtime, text, origin + Vec2{4, 26 + static_cast<float>(row) * 16.f} * size, size,
                      hovered ? Color{0.25f, 0.65f, 1.f} : Color{0.8f, 0.9f, 1});
            if (option.visualization)
                option.visualization(runtime, origin + Vec2{4, 26 + static_cast<float>(row) * 16.f} * size,
                                     size);
        }
    }
};

/// Draw collision outlines at a stable two-pixel width without covering the body.
///
/// Circle outline in world coordinates, as 32 line segments.
inline void draw_debug_circle(Runtime &runtime, Vec2 center, float radius, Color color) {
    constexpr int segments = 32;
    for (int i = 0; i < segments; ++i) {
        const float a = static_cast<float>(i) * 6.283185307f / static_cast<float>(segments),
                    b = static_cast<float>(i + 1) * 6.283185307f / static_cast<float>(segments);
        runtime.line(center + Vec2{std::cos(a), std::sin(a)} * radius,
                     center + Vec2{std::cos(b), std::sin(b)} * radius, 2.f / runtime.camera.pixels_per_unit,
                     color);
    }
}
/// Outline of a box with half extents `half`, rotated by `angle` radians, at a two-pixel
/// width; world coordinates.
inline void draw_debug_box(Runtime &runtime, Vec2 center, Vec2 half, float angle, Color color) {
    const std::array<Vec2, 4> corners{
        {{-half.x, -half.y}, {half.x, -half.y}, {half.x, half.y}, {-half.x, half.y}}};
    for (std::size_t i = 0; i < corners.size(); ++i)
        runtime.line(center + rotated(corners[i], angle), center + rotated(corners[(i + 1) % 4], angle),
                     2.f / runtime.camera.pixels_per_unit, color);
}

/// Debug widths and nonzero vectors remain legible across camera zoom levels.
///
/// Draws `vector` as an arrow from `position`, at least 1.5 world units and 24 screen
/// pixels long; a zero vector draws nothing.
inline void draw_debug_vector(Runtime &runtime, Vec2 position, Vec2 vector, Color color) {
    const float magnitude = length(vector);
    if (magnitude == 0)
        return;
    const Vec2 direction = vector / magnitude;
    const float size = std::max({magnitude, 1.5f, 24.f / runtime.camera.pixels_per_unit});
    const Vec2 end = position + direction * size;
    const float head = 5.f / runtime.camera.pixels_per_unit;
    const float width = 2.f / runtime.camera.pixels_per_unit;
    runtime.line(position, end, width, color);
    runtime.line(end, end - rotated(direction, 0.6f) * head, width, color);
    runtime.line(end, end - rotated(direction, -0.6f) * head, width, color);
}
} // namespace kinetra
