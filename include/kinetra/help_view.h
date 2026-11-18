#pragma once
#include "bitmap_text.h"
#include <span>
#include <string_view>

namespace kinetra {
struct HelpStyle {
    Vec2 reference_view{300, 210};
    float width{286}, expanded_height{};
    Vec2 origin{4, 4}, text_origin{10, 10};
    std::string_view collapsed{"H: SHOW CONTROLS"};
    float collapsed_width{108};
    Color background{0.025f, 0.035f, 0.06f};
    Color text{0.9f, 0.93f, 1.f}, heading{0.55f, 1.f, 0.35f};
};
/// Screen-space help content may include category headings and application sections.
/// Draw after world content; application bindings remain independent of the renderer.
inline void draw_help(Runtime &runtime, std::span<const std::string_view> lines, bool expanded,
                      const HelpStyle &style = {}) {
    const float scale = std::min({2.f, runtime.camera.viewport.x / style.reference_view.x,
                                  runtime.camera.viewport.y / style.reference_view.y});
    const std::size_t count = expanded ? lines.size() : 1;
    const Vec2 half{(expanded ? style.width : style.collapsed_width) * scale / 2,
                    (expanded && style.expanded_height > 0 ? style.expanded_height
                                                           : static_cast<float>(12 + 10 * count)) *
                        scale / 2};
    runtime.box(runtime.camera.screen_to_world(style.origin * scale + half),
                half / runtime.camera.pixels_per_unit, 0, style.background);
    for (std::size_t row = 0; row < count; ++row)
        draw_text(runtime, expanded ? lines[row] : style.collapsed,
                  (style.text_origin + Vec2{0, static_cast<float>(row) * 10.f}) * scale, scale,
                  row == 0 ? style.heading : style.text);
}
/// Optional visibility controller for applications using the default H binding.
struct HelpOverlay {
    bool expanded{};
    void update(const Input &input, Key toggle = Key::h) {
        if (input.hit(toggle))
            expanded = !expanded;
    }
    void draw(Runtime &runtime, std::span<const std::string_view> lines, const HelpStyle &style = {}) const {
        draw_help(runtime, lines, expanded, style);
    }
};
} // namespace kinetra
