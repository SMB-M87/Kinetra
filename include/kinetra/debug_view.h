#pragma once

#include "bitmap_text.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <span>
#include <string_view>

namespace kinetra {
/// One row of the debug menu. Set `enabled` for a check box or `action` for a button; the
/// other fields add read-only content to the row, and `decrease` and `increase` add buttons.
struct DebugOption {
    /// Row text; the viewed characters must outlive the menu's draw() calls.
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
    /// Optional `[-]` button at the right edge of the row, called when it is clicked
    /// instead of the row. With a button, keep the row text within 21 characters, so it
    /// ends before the button.
    std::function<void()> decrease{};
    /// Optional `[+]` button at the right edge of the row; see `decrease`.
    std::function<void()> increase{};
};

/// Shared screen-space debug controls. Layout is identical for drawing and picking;
/// all pointer input inside the panel is consumed, including held fire.
///
/// Positions are logical window pixels. The layout constants below are panel pixels, which
/// scale() converts to screen pixels.
struct DebugMenu {
    /// Width of the panel, in panel pixels.
    static constexpr float panel_width = 182;
    /// Height of the "DEBUG MENU" heading above the first row, in panel pixels.
    static constexpr float heading_height = 22;
    /// Height of one row, in panel pixels.
    static constexpr float row_height = 16;
    /// Distance from the panel's left edge, and from the top of a row, to its text, in
    /// panel pixels.
    static constexpr float text_margin = 4;
    /// Distance from the viewport's top-right corner to the default panel position, in
    /// panel pixels.
    static constexpr float screen_margin = 8;
    /// Width of a `[-]` or `[+]` button: three glyph cells, in panel pixels.
    static constexpr float stepper_width = 18;
    /// Left edge of the `[+]` button, flush with the right text margin, in panel pixels.
    static constexpr float increase_left = panel_width - text_margin - stepper_width;
    /// Left edge of the `[-]` button, one glyph cell before `[+]`, in panel pixels.
    static constexpr float decrease_left = increase_left - 6 - stepper_width;

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
        return screen_origin.x < 0
                   ? Vec2{viewport.x - (panel_width + screen_margin) * size, screen_margin * size}
                   : screen_origin;
    }
    /// Screen pixels per panel pixel for a viewport, at most 1.5.
    static float scale(Vec2 viewport) { return std::min({1.5f, viewport.x / 300.f, viewport.y / 260.f}); }
    /// Top edge of row `row` below the panel's top, in panel pixels.
    static float row_top(std::size_t row) { return heading_height + static_cast<float>(row) * row_height; }
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
        const bool captured = visible && point.x >= 0 && point.x <= panel_width && point.y >= 0 &&
                              point.y < row_top(options.size());
        if (captured && input.clicked && point.y >= heading_height) {
            auto &option = options[std::size_t((point.y - heading_height) / row_height)];
            switch (part_at(option, point.x)) {
            case RowPart::decrease:
                option.decrease();
                break;
            case RowPart::increase:
                option.increase();
                break;
            case RowPart::label:
                if (option.enabled)
                    *option.enabled = !*option.enabled;
                else if (option.action)
                    option.action();
                break;
            }
        }
        return captured;
    }
    /// Draws the panel in screen space when visible; pass the same options as to update().
    void draw(Runtime &runtime, std::span<const DebugOption> options) const {
        if (!visible)
            return;
        const float size = scale(runtime.camera.viewport);
        const Vec2 origin = this->origin(runtime.camera.viewport);
        const Vec2 half = Vec2{panel_width, row_top(options.size())} * size / 2;
        runtime.box(runtime.camera.screen_to_world(origin + half), half / runtime.camera.pixels_per_unit, 0,
                    {0.04f, 0.07f, 0.12f});
        draw_text(runtime, "DEBUG MENU", origin + Vec2{text_margin, 6} * size, size, {1, 1, 1});
        const Vec2 point = (pointer - origin) / size;
        for (std::size_t row = 0; row < options.size(); ++row) {
            const auto &option = options[row];
            std::string text = std::string(option.enabled ? (*option.enabled ? "[X] " : "[ ] ") : "") +
                               std::string(option.label);
            if (option.text)
                text += " " + option.text();
            if (option.number)
                text += " " + std::to_string(option.number());
            const bool on_row = point.x >= 0 && point.x <= panel_width && point.y >= row_top(row) &&
                                point.y < row_top(row + 1);
            const RowPart hovered = part_at(option, point.x);
            const auto color = [&](RowPart part, bool clickable) {
                return on_row && clickable && hovered == part ? highlight_color : text_color;
            };
            const float top = row_top(row) + text_margin;
            const Vec2 text_position = origin + Vec2{text_margin, top} * size;
            draw_text(runtime, text, text_position, size,
                      color(RowPart::label, option.enabled || option.action));
            if (option.decrease)
                draw_text(runtime, "[-]", origin + Vec2{decrease_left, top} * size, size,
                          color(RowPart::decrease, true));
            if (option.increase)
                draw_text(runtime, "[+]", origin + Vec2{increase_left, top} * size, size,
                          color(RowPart::increase, true));
            if (option.visualization)
                option.visualization(runtime, text_position, size);
        }
    }

  private:
    /// Text color of a clickable row or button under the pointer, and of everything else.
    static constexpr Color highlight_color{0.25f, 0.65f, 1.f}, text_color{0.8f, 0.9f, 1};
    /// The part of a row a click or the pointer is on.
    enum class RowPart { label, decrease, increase };
    /// The part of `option`'s row at panel x coordinate `x`. The gap between the two buttons
    /// is split between them and the `[+]` zone runs to the panel edge, so a near miss still
    /// hits the button it is closest to. A row without a button keeps that zone as label.
    static RowPart part_at(const DebugOption &option, float x) {
        const float gap = increase_left - (decrease_left + stepper_width);
        if (option.increase && x >= increase_left - gap / 2)
            return RowPart::increase;
        if (option.decrease && x >= decrease_left - gap / 2 && x < increase_left - gap / 2)
            return RowPart::decrease;
        return RowPart::label;
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

/// Layout of the frame-time graph in logical screen pixels.
struct FrameTimeGraph {
    /// Height of the bar area; a bar this tall is `ceiling_ms` or longer.
    static constexpr float height = 60;
    /// Frame time at the top of the bar area, in milliseconds.
    static constexpr float ceiling_ms = 50;
    /// Reference lines: one and two refreshes of a 60 Hz display.
    static constexpr float one_refresh_ms = 1000.f / 60, two_refreshes_ms = 2000.f / 60;
    /// Width of the graph: one pixel per frame held.
    static constexpr float width = float(FrameTimes::capacity);
    /// Top-left corner for `viewport`: the top-right corner, 12 pixels in. Kinetra-Lab keeps
    /// its toolbar and debug menu on the left and its statistics at the bottom.
    static Vec2 origin(Vec2 viewport) { return {viewport.x - width - 12, 8}; }
};

/// Draws `times` as a bar graph, newest on the right, with lines at 16.7 and 33.3 ms, and
/// below it the average and worst frame and how `runtime` paces frames (Runtime::pacing()),
/// in screen space at FrameTimeGraph::origin(). Bars up to one 60 Hz refresh (plus 5 % timer
/// jitter) are green, up to two yellow, longer ones red.
inline void draw_frame_times(Runtime &runtime, const FrameTimes &times) {
    const auto &camera = runtime.camera;
    const Vec2 origin = FrameTimeGraph::origin(camera.viewport);
    const float pixels_per_ms = FrameTimeGraph::height / FrameTimeGraph::ceiling_ms;
    // Screen-space rectangle from its top-left corner and size, like draw_text.
    const auto rectangle = [&](Vec2 corner, Vec2 size, Color color) {
        runtime.box(camera.screen_to_world(corner + size / 2), size / 2 / camera.pixels_per_unit, 0, color);
    };
    rectangle(origin - Vec2{4, 4}, {FrameTimeGraph::width + 8, FrameTimeGraph::height + 32},
              {0, 0, 0, 0.55f});
    const float bottom = origin.y + FrameTimeGraph::height;
    const float first = FrameTimeGraph::width - float(times.size());
    for (std::size_t i = 0; i < times.size(); ++i) {
        const float ms = float(times[i] * 1000);
        const float bar = std::min(ms, FrameTimeGraph::ceiling_ms) * pixels_per_ms;
        const Color color = ms <= FrameTimeGraph::one_refresh_ms * 1.05f     ? Color{0.3f, 0.9f, 0.4f}
                            : ms <= FrameTimeGraph::two_refreshes_ms * 1.05f ? Color{1, 0.85f, 0.2f}
                                                                             : Color{1, 0.3f, 0.25f};
        rectangle({origin.x + first + float(i), bottom - bar}, {1, bar}, color);
    }
    for (const float ms : {FrameTimeGraph::one_refresh_ms, FrameTimeGraph::two_refreshes_ms})
        rectangle({origin.x, bottom - ms * pixels_per_ms}, {FrameTimeGraph::width, 1}, {1, 1, 1, 0.45f});
    char text[64];
    std::snprintf(text, sizeof text, "FRAME AVG %.1f MS WORST %.1f MS", times.average() * 1000,
                  times.worst() * 1000);
    draw_text(runtime, text, {origin.x, bottom + 4}, 1, {1, 1, 1});
    // What spaces the frames decides how to read the bars: VSync gives even ones, a limiter
    // nearly even ones, and uncapped frames are as short as drawing allows.
    const int rate = static_cast<int>(std::lround(1 / runtime.frame_interval()));
    switch (runtime.pacing()) {
    case FramePacing::vsync:
        std::snprintf(text, sizeof text, "PACING VSYNC");
        break;
    case FramePacing::limited:
        std::snprintf(text, sizeof text, "PACING LIMIT %d HZ", rate);
        break;
    case FramePacing::uncapped:
        std::snprintf(text, sizeof text, "PACING UNCAPPED");
        break;
    }
    draw_text(runtime, text, {origin.x, bottom + 16}, 1, {0.7f, 0.8f, 1});
}
} // namespace kinetra
