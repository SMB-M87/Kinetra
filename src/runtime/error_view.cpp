#include <kinetra/runtime/error_view.h>

#include <kinetra/runtime/hud_text.h>
#include <kinetra/runtime/overlay_style.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>

namespace kinetra::error_panel {
namespace {
// The title bar, close button and scroll bar match the inspector's, so the two panels read
// as one kind.
constexpr float title_height = 24;
constexpr float padding = 8;
constexpr float close_size = 18;
constexpr std::string_view title = "ERROR";
// Screen pixels per character of the message, which uses the HUD text scale.
constexpr float character_width = 6 * hud::text_scale;
// One wheel notch moves one line: an error has few lines, each worth reading.
constexpr double lines_per_notch = 1;

struct Layout {
    Aabb bounds{};
    // Every line of the message; the panel shows `visible` of them at a time.
    std::vector<std::string> lines;
    std::size_t visible{};

    std::size_t last_first() const { return lines.size() - visible; }
    Vec2 close_origin() const { return {bounds.max.x - close_size - 3, bounds.min.y + 3}; }
    Aabb close_bounds() const {
        if (lines.empty())
            return {bounds.max, bounds.max};
        const Vec2 origin = close_origin();
        return {origin, origin + Vec2{close_size, close_size}};
    }
};

bool overlaps(Aabb a, Aabb b) {
    return a.min.x < b.max.x && b.min.x < a.max.x && a.min.y < b.max.y && b.min.y < a.max.y;
}

Layout layout(std::string_view message, Vec2 viewport, std::optional<Aabb> keep_clear) {
    Layout result;
    const Vec2 bottom_center{std::round(viewport.x / 2), std::round(viewport.y - hud::margin)};
    result.bounds = {bottom_center, bottom_center};
    // Whole pixels, so input and painting agree on every edge.
    const float width = std::floor(viewport.x - 2 * hud::margin);
    const float window_bottom = std::round(viewport.y - hud::margin);
    const auto lines_between = [](float top, float bottom) {
        return (bottom - top - title_height - padding) / hud::line_height;
    };
    const float most_lines = lines_between(hud::margin, window_bottom);
    if (message.empty() || width < 2 * padding + character_width || most_lines < 1)
        return result;
    const auto columns = static_cast<std::size_t>((width - 2 * padding) / character_width);
    const auto wrapped = hud::wrap(message, columns);
    const auto height_of = [](std::size_t lines) {
        return title_height + padding + static_cast<float>(lines) * hud::line_height;
    };
    const auto box = [&](float bottom, std::size_t lines) {
        return Aabb{{hud::margin, bottom - height_of(lines)}, {hud::margin + width, bottom}};
    };
    float bottom = window_bottom;
    // The panel leaves `keep_clear` free when it can: at the window's bottom if it misses the
    // region there, else directly above it if the whole message fits; only a longer message
    // covers it, since the panel then needs the window's height.
    if (keep_clear && overlaps(box(window_bottom, wrapped.size()), *keep_clear)) {
        const float above = std::floor(keep_clear->min.y - hud::margin);
        if (lines_between(hud::margin, above) >= static_cast<float>(wrapped.size()))
            bottom = above;
    }
    result.lines.assign(wrapped.begin(), wrapped.end());
    result.visible =
        std::min(result.lines.size(), static_cast<std::size_t>(lines_between(hud::margin, bottom)));
    result.bounds = box(bottom, result.visible);
    return result;
}

bool contains(Aabb box, Vec2 point) {
    return box.min.x <= point.x && point.x < box.max.x && box.min.y <= point.y && point.y < box.max.y;
}
} // namespace

Aabb bounds(std::string_view message, Vec2 viewport, std::optional<Aabb> keep_clear) {
    return layout(message, viewport, keep_clear).bounds;
}

Aabb close_bounds(std::string_view message, Vec2 viewport, std::optional<Aabb> keep_clear) {
    return layout(message, viewport, keep_clear).close_bounds();
}

std::vector<std::string> lines(std::string_view message, Vec2 viewport, std::optional<Aabb> keep_clear) {
    return layout(message, viewport, keep_clear).lines;
}

std::size_t visible_lines(std::string_view message, Vec2 viewport, std::optional<Aabb> keep_clear) {
    return layout(message, viewport, keep_clear).visible;
}

ErrorPanelInput update(const Input &input, std::string_view message, Vec2 viewport,
                       std::optional<Aabb> keep_clear, double first_line) {
    const auto panel = layout(message, viewport, keep_clear);
    const bool captured = !panel.lines.empty() && contains(panel.bounds, input.mouse);
    const double wheel = captured && std::isfinite(input.wheel) ? input.wheel : 0;
    // A positive wheel scrolls away from the user, toward the message's start.
    // Fractions add up, as in the inspector: a touchpad sends small steps every frame.
    const double start = std::isfinite(first_line) ? first_line : 0.0;
    const double first =
        std::clamp(start - wheel * lines_per_notch, 0.0, static_cast<double>(panel.last_first()));
    return {captured, captured && input.clicked && contains(panel.close_bounds(), input.mouse),
            panel.lines.empty() ? 0.0 : first};
}

void draw(Runtime &runtime, std::string_view message, Vec2 pointer, std::optional<Aabb> keep_clear,
          double first_line) {
    const auto panel = layout(message, runtime.camera.viewport, keep_clear);
    if (panel.lines.empty())
        return;
    const Vec2 corner = panel.bounds.min;
    const Vec2 size = panel.bounds.max - corner;
    overlay::draw_panel(runtime, corner, size, title_height, title, {}, 1);
    overlay::draw_sign_button(runtime, panel.close_origin(), {close_size, close_size}, 'X', 1,
                              contains(panel.close_bounds(), pointer));
    const auto first = static_cast<std::size_t>(
        std::clamp(std::floor(first_line), 0.0, static_cast<double>(panel.last_first())));
    // Each line is centered in its HUD line height, as on the HUD.
    const float inset = (hud::line_height - 7 * hud::text_scale) / 2;
    for (std::size_t i = 0; i < panel.visible; ++i) {
        const float top = title_height + padding / 2 + static_cast<float>(i) * hud::line_height + inset;
        draw_text(runtime, panel.lines[first + i], overlay::snapped(corner + Vec2{padding, top}),
                  hud::text_scale, hud::error_color);
    }
    if (panel.visible < panel.lines.size()) {
        // The inspector's scroll bar: a thin thumb whose length and place show the window.
        const float track = size.y - title_height - padding;
        const float thumb =
            std::max(3.f, track * static_cast<float>(panel.visible) / static_cast<float>(panel.lines.size()));
        const float offset =
            (track - thumb) * static_cast<float>(first) / static_cast<float>(panel.last_first());
        overlay::draw_rectangle(runtime,
                                {panel.bounds.max.x - 4, corner.y + title_height + padding / 2 + offset},
                                {2, thumb}, overlay::dim_color);
    }
}
} // namespace kinetra::error_panel
