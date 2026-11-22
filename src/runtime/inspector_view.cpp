#include <kinetra/runtime/inspector_view.h>

#include <kinetra/runtime/overlay_style.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string_view>

namespace kinetra {
namespace {
constexpr float max_width = 344;
constexpr float min_width = 96;
constexpr float title_height = 24;
constexpr float line_height = 13;
constexpr float padding = 8;
// The bitmap font advances six pixels per character at scale 1.
constexpr float character_width = 6;
constexpr float close_size = 18;
constexpr float scroll_lines_per_notch = 3;

struct Layout {
    Aabb bounds{};
    std::size_t columns{}, lines{}, visible_lines{};

    bool fits() const { return columns > 0; }
    double last_scroll() const { return static_cast<double>(lines - visible_lines); }
    Vec2 close_origin() const { return {bounds.max.x - close_size - 3, bounds.min.y + 3}; }
};

std::size_t wrapped_lines(std::size_t characters, std::size_t columns) {
    return characters == 0 ? 1 : 1 + (characters - 1) / columns;
}

Layout layout(const InspectorSnapshot &snapshot, Aabb area) {
    // Align inward so the same logical pixels are available to both input and painting.
    area.min = {std::ceil(area.min.x), std::ceil(area.min.y)};
    area.max = {std::floor(area.max.x), std::floor(area.max.y)};
    Layout result;
    result.bounds = {area.max, area.max};
    const float width = std::min(max_width, area.max.x - area.min.x);
    const float content_height = area.max.y - area.min.y - title_height - padding;
    if (width < min_width || content_height < line_height)
        return result;
    result.columns = static_cast<std::size_t>((width - 2 * padding) / character_width);
    const std::size_t label_columns = result.columns / 2;
    const std::size_t value_columns = result.columns - label_columns - 1;
    for (const auto &group : snapshot.groups) {
        if (!group.heading.empty())
            result.lines += wrapped_lines(group.heading.size(), result.columns);
        for (const auto &row : group.rows)
            result.lines += std::max(wrapped_lines(row.label.size(), label_columns),
                                     wrapped_lines(row.value.size(), value_columns));
    }
    result.visible_lines = std::min(result.lines, static_cast<std::size_t>(content_height / line_height));
    result.bounds = {{area.max.x - width, area.min.y},
                     {area.max.x, area.min.y + title_height + padding +
                                      static_cast<float>(result.visible_lines) * line_height}};
    return result;
}

bool contains(Aabb box, Vec2 point) {
    return box.min.x <= point.x && point.x < box.max.x && box.min.y <= point.y && point.y < box.max.y;
}

Aabb close_bounds(const Layout &panel) {
    const Vec2 origin = panel.close_origin();
    return {origin, origin + Vec2{close_size, close_size}};
}

std::string_view line_text(std::string_view text, std::size_t line, std::size_t columns) {
    const std::size_t start = line * columns;
    return start < text.size() ? text.substr(start, columns) : std::string_view{};
}
} // namespace

Aabb InspectorView::bounds(const InspectorSnapshot &snapshot, Aabb area) const {
    return layout(snapshot, area).bounds;
}

InspectorInput InspectorView::update(const Input &input, const InspectorSnapshot &snapshot, Aabb area) {
    pointer_ = input.mouse;
    const auto panel = layout(snapshot, area);
    const bool captured = contains(panel.bounds, pointer_);
    const double wheel = captured && std::isfinite(input.wheel) ? input.wheel : 0;
    first_line_ = std::clamp(first_line_ - wheel * scroll_lines_per_notch, 0.0, panel.last_scroll());
    return {captured, captured && input.clicked && contains(close_bounds(panel), pointer_)};
}

void InspectorView::draw(Runtime &runtime, const InspectorSnapshot &snapshot, Aabb area) const {
    const auto panel = layout(snapshot, area);
    if (!panel.fits())
        return;
    const Vec2 corner = panel.bounds.min;
    const Vec2 size = panel.bounds.max - corner;
    const auto title_columns =
        static_cast<std::size_t>((size.x - close_size - 2 * padding) / character_width);
    overlay::draw_panel(runtime, corner, size, title_height,
                        std::string_view(snapshot.title).substr(0, title_columns), {}, 1);
    overlay::draw_sign_button(runtime, panel.close_origin(), {close_size, close_size}, 'X', 1,
                              contains(close_bounds(panel), pointer_));

    const auto first = static_cast<std::size_t>(std::min(first_line_, panel.last_scroll()));
    const auto end = first + panel.visible_lines;
    const std::size_t label_columns = panel.columns / 2;
    const std::size_t value_columns = panel.columns - label_columns - 1;
    std::size_t line = 0;
    const auto draw_lines = [&](std::string_view label, std::string_view value, bool heading) {
        const auto columns = heading ? panel.columns : label_columns;
        const auto count =
            std::max(wrapped_lines(label.size(), columns), wrapped_lines(value.size(), value_columns));
        for (std::size_t i = std::max(line, first); i < std::min(line + count, end); ++i) {
            const float top = title_height + padding / 2 + static_cast<float>(i - first) * line_height;
            const Vec2 origin = overlay::snapped(corner + Vec2{padding, top});
            draw_text(runtime, line_text(label, i - line, columns), origin, 1,
                      heading ? overlay::bright_color : overlay::text_color);
            draw_text(runtime, line_text(value, i - line, value_columns),
                      origin + Vec2{static_cast<float>(label_columns + 1) * character_width, 0}, 1,
                      overlay::value_color);
        }
        line += count;
    };
    for (const auto &group : snapshot.groups) {
        if (!group.heading.empty())
            draw_lines(group.heading, {}, true);
        for (const auto &row : group.rows)
            draw_lines(row.label, row.value, false);
    }
    if (panel.visible_lines < panel.lines) {
        const float track = size.y - title_height - padding;
        const float thumb =
            std::max(3.f, track * static_cast<float>(panel.visible_lines) / static_cast<float>(panel.lines));
        const float offset =
            (track - thumb) * static_cast<float>(first) / static_cast<float>(panel.last_scroll());
        overlay::draw_rectangle(runtime,
                                {panel.bounds.max.x - 4, corner.y + title_height + padding / 2 + offset},
                                {2, thumb}, overlay::dim_color);
    }
}

void InspectorView::reset() {
    first_line_ = 0;
    pointer_ = {-1, -1};
}
} // namespace kinetra
