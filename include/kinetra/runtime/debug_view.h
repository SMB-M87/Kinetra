#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <kinetra/runtime/bitmap_text.h>
#include <kinetra/runtime/overlay_style.h>
#include <kinetra/view/debug_menu.h>
#include <span>
#include <string>
#include <string_view>

namespace kinetra {
/// Whether this frame's input is the debug shortcut, Shift+Ctrl: true on the frame the
/// second of the two keys goes down, in either order. A key that is pressed and released
/// within one frame counts as down for that frame.
inline bool debug_chord(const Input &input) {
    const bool shift = input.down(Key::shift) || input.hit(Key::shift);
    const bool control = input.down(Key::control) || input.hit(Key::control);
    return (input.hit(Key::control) && shift) || (input.hit(Key::shift) && control);
}

/// Shared screen-space debug controls: a panel that draws DebugEntry rows and maps clicks to
/// them. Layout is identical for drawing and picking; all pointer input inside the panel is
/// consumed, including held fire.
///
/// Positions are logical window pixels. The layout constants below are panel pixels, which
/// scale() converts to screen pixels. The panel is not opened by a key of its own:
/// SettingsOverlay::update_debug() shows and hides it, and so may any other owner through
/// `visible`.
struct DebugMenu {
    /// Width of the panel, in panel pixels.
    static constexpr float panel_width = 216;
    /// Height of the title bar above the first row, in panel pixels.
    static constexpr float heading_height = 20;
    /// Height of one row, in panel pixels.
    static constexpr float row_height = 16;
    /// Distance from the panel's left and right edge to its content, in panel pixels.
    static constexpr float text_margin = 6;
    /// Distance from the top of a row to its text, in panel pixels.
    static constexpr float text_top = 4;
    /// Indentation per nesting level, and the width of the column that holds a row's state
    /// mark before its label, in panel pixels.
    static constexpr float indent = 10, mark_width = 10;
    /// Distance from the viewport's top-right corner to the default panel position, in
    /// panel pixels.
    static constexpr float screen_margin = 8;
    /// Width of a `[-]` or `[+]` button and the gap between the two, in panel pixels.
    static constexpr float stepper_width = 14, stepper_gap = 2;
    /// Left edge of the `[+]` button, flush with the right margin, in panel pixels.
    static constexpr float increase_left = panel_width - text_margin - stepper_width;
    /// Left edge of the `[-]` button, in panel pixels.
    static constexpr float decrease_left = increase_left - stepper_gap - stepper_width;
    /// Side of a row's color swatch at the right margin, in panel pixels.
    static constexpr float swatch_size = 7;
    /// Space below the last row, in panel pixels.
    static constexpr float bottom_padding = 4;

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
    /// Screen pixels per panel pixel for a viewport: at most 1.5, and smaller on a low window,
    /// so about as many rows fit a 540-pixel window as an 800-pixel one.
    static float scale(Vec2 viewport) { return std::min({1.5f, viewport.x / 300.f, viewport.y / 420.f}); }
    /// Top edge of row `row` below the panel's top, in panel pixels.
    static constexpr float row_top(std::size_t row) {
        return heading_height + static_cast<float>(row) * row_height;
    }
    /// Height of a panel with `rows` rows, in panel pixels.
    static constexpr float height(std::size_t rows) { return row_top(rows) + bottom_padding; }
    /// Left edge of the label of a row at nesting level `depth`, in panel pixels. The state
    /// mark sits `mark_width` before it.
    static constexpr float label_left(int depth) {
        return text_margin + static_cast<float>(std::max(depth, 0)) * indent + mark_width;
    }

    /// Applies one frame of input: a click on a row runs its `[-]` or `[+]` function or its
    /// action. Returns true when the pointer is over the visible panel, so the caller must
    /// ignore that pointer input.
    bool update(const Input &input, Vec2 viewport, std::span<const DebugEntry> entries) {
        pointer = input.mouse;
        const Vec2 point = (input.mouse - origin(viewport)) / scale(viewport);
        const bool captured = visible && point.x >= 0 && point.x <= panel_width && point.y >= 0 &&
                              point.y < height(entries.size());
        if (captured && input.clicked && point.y >= heading_height && point.y < row_top(entries.size())) {
            const auto &entry = entries[static_cast<std::size_t>((point.y - heading_height) / row_height)];
            switch (part_at(entry, point.x)) {
            case RowPart::decrease:
                entry.decrease();
                break;
            case RowPart::increase:
                entry.increase();
                break;
            case RowPart::label:
                if (entry.action)
                    entry.action();
                break;
            }
        }
        return captured;
    }

    /// Draws the panel in screen space when visible; pass the same entries as to update().
    void draw(Runtime &runtime, std::span<const DebugEntry> entries) const {
        if (!visible)
            return;
        const float size = scale(runtime.camera.viewport);
        const Vec2 origin = this->origin(runtime.camera.viewport);
        // Screen position of a point given in panel pixels, on a whole pixel.
        const auto at = [&](float x, float y) { return overlay::snapped(origin + Vec2{x, y} * size); };
        overlay::draw_panel(runtime, origin, Vec2{panel_width, height(entries.size())} * size,
                            heading_height * size, "DEBUG", "SHIFT+CTRL", size);
        const Vec2 point = (pointer - origin) / size;
        for (std::size_t row = 0; row < entries.size(); ++row) {
            const auto &entry = entries[row];
            const float top = row_top(row);
            const bool on_row =
                point.x >= 0 && point.x <= panel_width && point.y >= top && point.y < top + row_height;
            const RowPart hovered = part_at(entry, point.x);
            const bool heading = entry.kind == DebugEntryKind::group && entry.depth == 0;
            const bool lit = on_row && hovered == RowPart::label && static_cast<bool>(entry.action);
            // The band stays inside the outline, which is one panel pixel wide.
            if (lit || heading)
                overlay::draw_rectangle(runtime, at(1, top), Vec2{panel_width - 2, row_height} * size,
                                        lit ? overlay::band_color : overlay::section_color);
            if (entry.rule_above)
                overlay::draw_rectangle(runtime, at(text_margin, top),
                                        {(panel_width - 2 * text_margin) * size, std::max(1.f, size)},
                                        overlay::rule_color);
            const float label = label_left(entry.depth);
            const bool active = entry.active && entry.active();
            draw_mark(runtime, entry.kind, active, at(label - mark_width, top + text_top), size);
            // Dim text is for a row that only shows something; a row with buttons is live.
            const bool adjustable = entry.increase || entry.decrease;
            draw_text(runtime, entry.label, at(label, top + text_top), size,
                      lit || heading               ? overlay::bright_color
                      : entry.action || adjustable ? overlay::text_color
                                                   : overlay::dim_color);
            float right = panel_width - text_margin;
            if (adjustable) {
                const Vec2 button{stepper_width * size, (row_height - 4) * size};
                if (entry.decrease)
                    overlay::draw_sign_button(runtime, at(decrease_left, top + 2), button, '-', size,
                                              on_row && hovered == RowPart::decrease);
                if (entry.increase)
                    overlay::draw_sign_button(runtime, at(increase_left, top + 2), button, '+', size,
                                              on_row && hovered == RowPart::increase);
                right = decrease_left - text_margin;
            }
            if (entry.swatch) {
                right -= swatch_size;
                overlay::draw_rectangle(runtime, at(right, top + text_top),
                                        Vec2{swatch_size, swatch_size} * size, *entry.swatch);
                right -= text_margin;
            }
            if (entry.value) {
                const std::string value = entry.value();
                draw_text(runtime, value, at(right - text_width(value), top + text_top), size,
                          overlay::value_color);
            }
        }
    }

  private:
    /// The part of a row a click or the pointer is on.
    enum class RowPart { label, decrease, increase };
    /// The part of `entry`'s row at panel x coordinate `x`. The gap between the two buttons
    /// is split between them and the `[+]` zone runs to the panel edge, so a near miss still
    /// hits the button it is closest to. A row without a button keeps that zone as label.
    static RowPart part_at(const DebugEntry &entry, float x) {
        const float middle = increase_left - stepper_gap / 2;
        if (entry.increase && x >= middle)
            return RowPart::increase;
        if (entry.decrease && x >= decrease_left - stepper_gap / 2 && x < middle)
            return RowPart::decrease;
        return RowPart::label;
    }
    /// Draws the state mark of a row of `kind` at screen position `corner`; a button has none.
    static void draw_mark(Runtime &runtime, DebugEntryKind kind, bool active, Vec2 corner, float size) {
        switch (kind) {
        case DebugEntryKind::button:
            break;
        case DebugEntryKind::check:
            draw_glyph(runtime, active ? overlay::check_on : overlay::check_off, corner, size,
                       active ? overlay::on_color : overlay::dim_color);
            break;
        case DebugEntryKind::choice:
            draw_glyph(runtime, active ? overlay::radio_on : overlay::radio_off, corner, size,
                       active ? overlay::on_color : overlay::dim_color);
            break;
        case DebugEntryKind::group:
            draw_glyph(runtime, active ? overlay::arrow_open : overlay::arrow_closed, corner, size,
                       overlay::accent_color);
            break;
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

/// Layout of the frame-time graph in logical screen pixels.
struct FrameTimeGraph {
    /// Height of the bar area; a bar this tall is `ceiling_ms` or longer.
    static constexpr float height = 60;
    /// Height including the background and captions, in logical pixels.
    static constexpr float total_height = height + 32;
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
/// in screen space at `origin` (logical pixels). A positive `scale` sizes the entire graph.
/// Bars up to one 60 Hz refresh (plus 5 % timer jitter) are green, up to two yellow, longer ones red.
inline void draw_frame_times(Runtime &runtime, const FrameTimes &times, Vec2 origin, float scale = 1) {
    const auto &camera = runtime.camera;
    const float pixels_per_ms = FrameTimeGraph::height / FrameTimeGraph::ceiling_ms;
    // Screen-space rectangle from its top-left corner and size, like draw_text.
    const auto rectangle = [&](Vec2 corner, Vec2 size, Color color) {
        corner = origin + corner * scale;
        size = size * scale;
        runtime.box(camera.screen_to_world(corner + size / 2), size / 2 / camera.pixels_per_unit, 0, color);
    };
    rectangle({-4, -4}, {FrameTimeGraph::width + 8, FrameTimeGraph::total_height}, {0, 0, 0, 0.55f});
    const float bottom = FrameTimeGraph::height;
    const float first = FrameTimeGraph::width - float(times.size());
    for (std::size_t i = 0; i < times.size(); ++i) {
        const float ms = float(times[i] * 1000);
        const float bar = std::min(ms, FrameTimeGraph::ceiling_ms) * pixels_per_ms;
        const Color color = ms <= FrameTimeGraph::one_refresh_ms * 1.05f     ? Color{0.3f, 0.9f, 0.4f}
                            : ms <= FrameTimeGraph::two_refreshes_ms * 1.05f ? Color{1, 0.85f, 0.2f}
                                                                             : Color{1, 0.3f, 0.25f};
        rectangle({first + float(i), bottom - bar}, {1, bar}, color);
    }
    for (const float ms : {FrameTimeGraph::one_refresh_ms, FrameTimeGraph::two_refreshes_ms})
        rectangle({0, bottom - ms * pixels_per_ms}, {FrameTimeGraph::width, 1}, {1, 1, 1, 0.45f});
    char text[64];
    std::snprintf(text, sizeof text, "FRAME AVG %.1f MS WORST %.1f MS", times.average() * 1000,
                  times.worst() * 1000);
    draw_text(runtime, text, origin + Vec2{0, bottom + 4} * scale, scale, {1, 1, 1});
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
    draw_text(runtime, text, origin + Vec2{0, bottom + 16} * scale, scale, {0.7f, 0.8f, 1});
}

/// Draws the frame-time graph at its default top-right position.
inline void draw_frame_times(Runtime &runtime, const FrameTimes &times) {
    draw_frame_times(runtime, times, FrameTimeGraph::origin(runtime.camera.viewport));
}
} // namespace kinetra
