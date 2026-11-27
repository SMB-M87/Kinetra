#include <kinetra/runtime/error_view.h>

#include <kinetra/runtime/hud_text.h>
#include <kinetra/runtime/overlay_style.h>

#include <cmath>
#include <cstddef>

namespace kinetra::error_panel {
namespace {
// The title bar and close button match the inspector's, so the two panels read as one kind.
constexpr float title_height = 24;
constexpr float padding = 8;
constexpr float close_size = 18;
constexpr std::string_view title = "ERROR";
// Screen pixels per character of the message, which uses the HUD text scale.
constexpr float character_width = 6 * hud::text_scale;

struct Layout {
    Aabb bounds{};
    std::vector<std::string> lines;

    Vec2 close_origin() const { return {bounds.max.x - close_size - 3, bounds.min.y + 3}; }
    Aabb close_bounds() const {
        if (lines.empty())
            return {bounds.max, bounds.max};
        const Vec2 origin = close_origin();
        return {origin, origin + Vec2{close_size, close_size}};
    }
};

Layout layout(std::string_view message, Vec2 viewport) {
    Layout result;
    const Vec2 bottom_center{std::round(viewport.x / 2), std::round(viewport.y - hud::margin)};
    result.bounds = {bottom_center, bottom_center};
    // Whole pixels, so input and painting agree on every edge.
    const float width = std::floor(viewport.x - 2 * hud::margin);
    const float most_lines = (viewport.y - 2 * hud::margin - title_height - padding) / hud::line_height;
    if (message.empty() || width < 2 * padding + character_width || most_lines < 1)
        return result;
    const auto columns = static_cast<std::size_t>((width - 2 * padding) / character_width);
    result.lines = hud::error_lines(message, columns, static_cast<int>(most_lines));
    const float height = title_height + padding + static_cast<float>(result.lines.size()) * hud::line_height;
    const float bottom = std::round(viewport.y - hud::margin);
    result.bounds = {{hud::margin, bottom - height}, {hud::margin + width, bottom}};
    return result;
}

bool contains(Aabb box, Vec2 point) {
    return box.min.x <= point.x && point.x < box.max.x && box.min.y <= point.y && point.y < box.max.y;
}
} // namespace

Aabb bounds(std::string_view message, Vec2 viewport) {
    return layout(message, viewport).bounds;
}

Aabb close_bounds(std::string_view message, Vec2 viewport) {
    return layout(message, viewport).close_bounds();
}

std::vector<std::string> lines(std::string_view message, Vec2 viewport) {
    return layout(message, viewport).lines;
}

ErrorPanelInput update(const Input &input, std::string_view message, Vec2 viewport) {
    const auto panel = layout(message, viewport);
    const bool captured = !panel.lines.empty() && contains(panel.bounds, input.mouse);
    return {captured, captured && input.clicked && contains(panel.close_bounds(), input.mouse)};
}

void draw(Runtime &runtime, std::string_view message, Vec2 pointer) {
    const auto panel = layout(message, runtime.camera.viewport);
    if (panel.lines.empty())
        return;
    const Vec2 corner = panel.bounds.min;
    const Vec2 size = panel.bounds.max - corner;
    overlay::draw_panel(runtime, corner, size, title_height, title, {}, 1);
    overlay::draw_sign_button(runtime, panel.close_origin(), {close_size, close_size}, 'X', 1,
                              contains(panel.close_bounds(), pointer));
    // Each line is centered in its HUD line height, as on the HUD.
    const float inset = (hud::line_height - 7 * hud::text_scale) / 2;
    for (std::size_t i = 0; i < panel.lines.size(); ++i) {
        const float top = title_height + padding / 2 + static_cast<float>(i) * hud::line_height + inset;
        draw_text(runtime, panel.lines[i], overlay::snapped(corner + Vec2{padding, top}), hud::text_scale,
                  hud::error_color);
    }
}
} // namespace kinetra::error_panel
