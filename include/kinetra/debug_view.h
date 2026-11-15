#pragma once

#include "bitmap_text.h"
#include <functional>
#include <span>
#include <string_view>

namespace kinetra {
struct DebugOption {
    std::string_view label;
    bool *enabled{}; // Borrowed; must outlive update/draw.
    std::function<void()> action{};
    std::function<std::string()> text{};
    std::function<double()> number{};
    std::function<void(Runtime &, Vec2, float)> visualization{};
};

/// Shared screen-space debug controls. Layout is identical for drawing and picking;
/// all pointer input inside the panel is consumed, including held fire.
struct DebugMenu {
    bool visible{};
    static float scale(Vec2 viewport) { return std::min({1.5f, viewport.x / 300.f, viewport.y / 260.f}); }
    bool update(const Input &input, Vec2 viewport, std::span<DebugOption> options,
                Key toggle = Key::backquote) {
        if (input.hit(toggle))
            visible = !visible;
        const float size = scale(viewport);
        const Vec2 point = (input.mouse - Vec2{viewport.x - 190 * size, 8 * size}) / size;
        const bool captured =
            visible && point.x >= 0 && point.x <= 182 && point.y >= 0 && point.y < 22 + options.size() * 16;
        if (captured && input.clicked && point.y >= 22) {
            const auto row = std::size_t((point.y - 22) / 16);
            if (options[row].enabled)
                *options[row].enabled = !*options[row].enabled;
            else if (options[row].action)
                options[row].action();
        }
        return captured;
    }
    void draw(Runtime &runtime, std::span<const DebugOption> options) const {
        if (!visible)
            return;
        const float size = scale(runtime.camera.viewport);
        const Vec2 origin{runtime.camera.viewport.x - 190 * size, 8 * size};
        const Vec2 half{91 * size, (22 + options.size() * 16) * size / 2};
        runtime.box(runtime.camera.screen_to_world(origin + half), half / runtime.camera.pixels_per_unit, 0,
                    {0.04f, 0.07f, 0.12f});
        draw_text(runtime, "DEBUG MENU", origin + Vec2{4, 6} * size, size, {1, 1, 1});
        for (std::size_t row = 0; row < options.size(); ++row) {
            const auto &option = options[row];
            std::string text = std::string(option.enabled ? (*option.enabled ? "[X] " : "[ ] ") : "") +
                               std::string(option.label);
            if (option.text)
                text += " " + option.text();
            if (option.number)
                text += " " + std::to_string(option.number());
            draw_text(runtime, text, origin + Vec2{4, 26 + row * 16.f} * size, size, {0.8f, 0.9f, 1});
            if (option.visualization)
                option.visualization(runtime, origin + Vec2{4, 26 + row * 16.f} * size, size);
        }
    }
};

/// Draw collision outlines at a stable two-pixel width without covering the body.
inline void draw_debug_circle(Runtime &runtime, Vec2 center, float radius, Color color) {
    constexpr int segments = 32;
    for (int i = 0; i < segments; ++i) {
        const float a = i * 6.283185307f / segments, b = (i + 1) * 6.283185307f / segments;
        runtime.line(center + Vec2{std::cos(a), std::sin(a)} * radius,
                     center + Vec2{std::cos(b), std::sin(b)} * radius, 2.f / runtime.camera.pixels_per_unit,
                     color);
    }
}
inline void draw_debug_box(Runtime &runtime, Vec2 center, Vec2 half, float angle, Color color) {
    const std::array<Vec2, 4> corners{
        {{-half.x, -half.y}, {half.x, -half.y}, {half.x, half.y}, {-half.x, half.y}}};
    for (std::size_t i = 0; i < corners.size(); ++i)
        runtime.line(center + rotated(corners[i], angle), center + rotated(corners[(i + 1) % 4], angle),
                     2.f / runtime.camera.pixels_per_unit, color);
}

/// Debug widths and nonzero vectors remain legible across camera zoom levels.
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
