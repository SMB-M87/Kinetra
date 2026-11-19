#pragma once
// The drawn style the screen-space overlays share: colors, state marks and panel pieces.
// The settings panel and the debug menu draw with these, so they look like one program.

#include "bitmap_text.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace kinetra::overlay {
/// Panel body and title bar.
inline constexpr Color panel_color{0.043f, 0.059f, 0.098f}, title_color{0.078f, 0.110f, 0.176f};
/// Panel outline, and the line that sets rows apart.
inline constexpr Color border_color{0.196f, 0.275f, 0.400f}, rule_color{0.137f, 0.188f, 0.282f};
/// Background of the row under the pointer, and of a section heading.
inline constexpr Color band_color{0.106f, 0.176f, 0.290f}, section_color{0.063f, 0.086f, 0.141f};
/// Row text, text of the row or button under the pointer and of headings, and text that
/// cannot be clicked.
inline constexpr Color text_color{0.82f, 0.89f, 1}, bright_color{1, 1, 1}, dim_color{0.50f, 0.58f, 0.72f};
/// The button under the pointer, a value, and a mark that is on.
inline constexpr Color accent_color{0.25f, 0.65f, 1}, value_color{0.55f, 0.85f, 1},
    on_color{0.35f, 0.90f, 0.55f};
/// Background of a button that is not under the pointer.
inline constexpr Color button_color{0.106f, 0.153f, 0.243f};

/// State marks in the bitmap font's cell: a group that is closed or open, a check box and
/// a radio mark, each off and on.
inline constexpr Glyph arrow_closed{0, 8, 12, 14, 12, 8, 0}, arrow_open{0, 0, 31, 14, 4, 0, 0};
inline constexpr Glyph check_off{0, 31, 17, 17, 17, 31, 0}, check_on{0, 31, 31, 31, 31, 31, 0};
inline constexpr Glyph radio_off{0, 14, 17, 17, 17, 14, 0}, radio_on{0, 14, 31, 31, 31, 14, 0};

/// `point` moved to the nearest whole pixel. Text and marks start on whole pixels, so a font
/// pixel never has an edge on a pixel center, where it would be drawn or not by chance.
inline Vec2 snapped(Vec2 point) {
    return {std::round(point.x), std::round(point.y)};
}

/// Filled rectangle in logical screen pixels, from its top-left corner. Edges are rounded to
/// whole pixels, so neighboring pieces meet without a seam; a rectangle that rounds to
/// nothing is not drawn.
inline void draw_rectangle(Runtime &runtime, Vec2 corner, Vec2 size, Color color) {
    const Vec2 low{std::round(corner.x), std::round(corner.y)};
    const Vec2 high{std::round(corner.x + size.x), std::round(corner.y + size.y)};
    if (high.x <= low.x || high.y <= low.y)
        return;
    runtime.box(runtime.camera.screen_to_world((low + high) / 2),
                (high - low) / 2 / runtime.camera.pixels_per_unit, 0, color);
}

/// Outline of a rectangle, drawn inside it, `thickness` screen pixels wide and at least one.
inline void draw_outline(Runtime &runtime, Vec2 corner, Vec2 size, float thickness, Color color) {
    const float t = std::max(1.f, std::round(thickness));
    draw_rectangle(runtime, corner, {size.x, t}, color);
    draw_rectangle(runtime, corner + Vec2{0, size.y - t}, {size.x, t}, color);
    draw_rectangle(runtime, corner, {t, size.y}, color);
    draw_rectangle(runtime, corner + Vec2{size.x - t, 0}, {t, size.y}, color);
}

/// A panel: body, a title bar of `title_height` with `title` at its left and the dimmer
/// `hint` at its right, and an outline. Positions and sizes are screen pixels; `scale` is
/// the screen pixels per font pixel of the text, which sits `scale` × 6 from the edges.
inline void draw_panel(Runtime &runtime, Vec2 corner, Vec2 size, float title_height, std::string_view title,
                       std::string_view hint, float scale) {
    draw_rectangle(runtime, corner, size, panel_color);
    draw_rectangle(runtime, corner, {size.x, title_height}, title_color);
    const float text_top = (title_height - 7 * scale) / 2;
    draw_text(runtime, title, snapped(corner + Vec2{6 * scale, text_top}), scale, bright_color);
    draw_text(runtime, hint, snapped(corner + Vec2{size.x - (6 + text_width(hint)) * scale, text_top}), scale,
              dim_color);
    draw_outline(runtime, corner, size, scale, border_color);
}

/// A small button showing `sign`, a character of the bitmap font, centered in a box of
/// `size` screen pixels; `lit` draws it as the button under the pointer.
inline void draw_sign_button(Runtime &runtime, Vec2 corner, Vec2 size, char sign, float scale, bool lit) {
    draw_rectangle(runtime, corner, size, lit ? accent_color : button_color);
    draw_glyph(runtime, glyph(sign),
               snapped(corner + Vec2{(size.x - 5 * scale) / 2, (size.y - 7 * scale) / 2}), scale,
               lit ? panel_color : text_color);
}

/// A button with centered text and an outline, in a box of `size` screen pixels; `lit` draws
/// it as the button under the pointer.
inline void draw_text_button(Runtime &runtime, Vec2 corner, Vec2 size, std::string_view text, float scale,
                             bool lit) {
    draw_rectangle(runtime, corner, size, lit ? band_color : button_color);
    draw_outline(runtime, corner, size, scale, lit ? accent_color : border_color);
    draw_text(runtime, text,
              snapped(corner + Vec2{(size.x - text_width(text) * scale) / 2, (size.y - 7 * scale) / 2}),
              scale, lit ? bright_color : text_color);
}
} // namespace kinetra::overlay
