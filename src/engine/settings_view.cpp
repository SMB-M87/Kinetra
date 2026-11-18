#include <kinetra/bitmap_text.h>
#include <kinetra/settings_view.h>

#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace kinetra {
namespace {
// Layout in logical panel pixels. The values reproduce the panel Kinetra-Lab drew before
// the overlay moved here (ADR 0013), so its screenshots and settings tests stay valid.
constexpr float title_y = 12;
constexpr float first_row_y = 40;
constexpr float row_spacing = 28;
constexpr float section_gap = 4;
constexpr float line_spacing = 14;
constexpr float hint_spacing = 28;
constexpr float bottom_margin = 50;
// A row is clickable from 4 pixels above its text to 11 pixels below the 7-pixel glyphs.
constexpr float band_above = 4;
constexpr float band_below = 18;
// Clicks outside this column of the open panel are captured but do nothing.
constexpr float column_left = 10;
constexpr float column_right = 174;

/// A horizontal button: drawn at `text_x`, clickable in [left, right).
struct Button {
    std::string_view text;
    float text_x, left, right;
};
constexpr Button zoom_in{"ZOOM IN", 36, 32, 81};
constexpr Button zoom_out{"ZOOM OUT", 101, 97, 152};
constexpr Button launcher_button{"LAUNCHER", 27.5f, 24.5f, 77.5f};
constexpr Button resume_button{"RESUME", 86.5f, 83.5f, 124.5f};
constexpr Button exit_button{"EXIT", 133.5f, 130.5f, 159.5f};

// Closed-state toolbar, in screen pixels at the top-left corner of the window.
constexpr float toolbar_height = 24;
constexpr float settings_button_right = 64;
constexpr float debug_button_right = 112;
constexpr Vec2 debug_menu_origin{8, 32};

constexpr Color panel_color{0.025f, 0.035f, 0.06f};
constexpr Color text_color{0.9f, 0.93f, 1};
constexpr Color toolbar_text_color{1, 1, 1};
constexpr Color highlight_color{0.25f, 0.65f, 1.f};

bool in_band(Vec2 point, float y) {
    return point.y >= y - band_above && point.y < y + band_below;
}
bool on_toolbar(Vec2 mouse, float left, float right) {
    return mouse.x >= left && mouse.x < right && mouse.y >= 0 && mouse.y < toolbar_height;
}
bool is_section(const SettingsRow &row) {
    return std::holds_alternative<HelpSection>(row);
}
float centered_x(std::string_view text) {
    // Glyph cells are 6 pixels wide; the last cell has no trailing gap.
    return (SettingsOverlay::panel_width - (static_cast<float>(text.size()) * 6.f - 1.f)) / 2;
}
} // namespace

SettingsOverlay::SettingsOverlay(std::vector<SettingsRow> rows, const bool *debug_mode)
    : rows_(std::move(rows)), expanded_(rows_.size(), false), debug_mode_(debug_mode) {
    bool sections = false;
    for (const auto &row : rows_) {
        if (is_section(row))
            sections = true;
        else if (sections)
            throw std::invalid_argument("Settings rows must precede help sections");
        if (const auto *zoom = std::get_if<ZoomRow>(&row); zoom && !zoom->limits)
            throw std::invalid_argument("Zoom row without limits");
        if (const auto *toggle = std::get_if<ToggleRow>(&row); toggle && !toggle->value)
            throw std::invalid_argument("Toggle row without a flag");
    }
}

float SettingsOverlay::scale(Vec2 viewport) {
    // 204 x 418 fits the tallest Lab panel with a margin; the scale never depends on the
    // expanded state, so opening a help section does not resize the panel.
    return std::min({2.f, viewport.x / 204.f, viewport.y / 418.f});
}

std::vector<float> SettingsOverlay::layout() const {
    std::vector<float> positions;
    positions.reserve(rows_.size());
    float y = first_row_y;
    bool sections = false;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const auto *section = std::get_if<HelpSection>(&rows_[i]);
        if (section && !sections) {
            y += section_gap;
            sections = true;
        }
        positions.push_back(y);
        y += row_spacing;
        // Expanded lines start one line below the toggle; the next toggle follows one
        // line below the last line.
        if (section && expanded_[i] && !section->lines.empty())
            y += line_spacing * static_cast<float>(section->lines.size() - 1);
    }
    return positions;
}

float SettingsOverlay::actions_y(const std::vector<float> &positions) const {
    if (positions.empty())
        return first_row_y;
    const std::size_t last = positions.size() - 1;
    float y = positions[last] + row_spacing;
    if (const auto *section = std::get_if<HelpSection>(&rows_[last]); section && expanded_[last])
        y += line_spacing * static_cast<float>(section->lines.size());
    return y;
}

float SettingsOverlay::actions_y() const {
    return actions_y(layout());
}

Vec2 SettingsOverlay::half_extent() const {
    return {panel_width / 2, (actions_y() + bottom_margin) / 2};
}

float SettingsOverlay::row_y(std::size_t index) const {
    return layout().at(index);
}

bool SettingsOverlay::expanded(std::size_t index) const {
    return expanded_.at(index);
}

bool SettingsOverlay::update(const Input &input, Camera2D &camera) {
    pointer_ = input.mouse;
    const bool was_visible = visible_;
    // Positions are taken before this click changes any section, as drawn last frame.
    const auto positions = layout();
    const float actions = actions_y(positions);
    const Vec2 half{panel_width / 2, (actions + bottom_margin) / 2};
    const Vec2 point = (input.mouse - camera.viewport / 2) / scale(camera.viewport) + half;
    if (input.hit(Key::control) ||
        (!visible_ && input.clicked && on_toolbar(input.mouse, 0, settings_button_right))) {
        visible_ = !visible_;
        return was_visible || visible_;
    }
    if (!visible_ || !input.clicked || point.x < column_left || point.x > column_right)
        return was_visible || visible_;
    auto on = [&](const Button &button) { return point.x >= button.left && point.x < button.right; };
    if (in_band(point, actions)) {
        if (on(launcher_button))
            request_ = SessionRequest::menu;
        else if (on(resume_button))
            visible_ = false;
        else if (on(exit_button))
            request_ = SessionRequest::exit;
        return true;
    }
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (!in_band(point, positions[i]))
            continue;
        if (const auto *zoom_row = std::get_if<ZoomRow>(&rows_[i])) {
            const ZoomLimits limits = zoom_row->limits();
            if (on(zoom_in))
                camera.pixels_per_unit =
                    std::clamp(camera.pixels_per_unit * zoom_factor, limits.minimum, limits.maximum);
            else if (on(zoom_out))
                camera.pixels_per_unit =
                    std::clamp(camera.pixels_per_unit / zoom_factor, limits.minimum, limits.maximum);
        } else if (const auto *toggle = std::get_if<ToggleRow>(&rows_[i]))
            *toggle->value = !*toggle->value;
        else
            expanded_[i] = !expanded_[i];
        break;
    }
    return true;
}

bool SettingsOverlay::update_debug(const Input &input, Vec2 viewport, DebugMenu &menu,
                                   std::span<DebugOption> options) const {
    menu.screen_origin = debug_menu_origin;
    const bool captured = menu.update(input, viewport, options);
    if (on_toolbar(input.mouse, settings_button_right, debug_button_right)) {
        if (input.clicked)
            menu.visible = !menu.visible;
        return true;
    }
    return captured;
}

void SettingsOverlay::draw(Runtime &runtime) const {
    const Camera2D &camera = runtime.camera;
    if (!visible_) {
        const bool debug = debug_mode_ && *debug_mode_;
        const float width = debug ? debug_button_right : settings_button_right;
        runtime.box(camera.screen_to_world({width / 2, toolbar_height / 2}),
                    Vec2{width / 2, toolbar_height / 2} / camera.pixels_per_unit, 0, panel_color);
        draw_text(runtime, "SETTINGS", {8, 8}, 1,
                  on_toolbar(pointer_, 0, settings_button_right) ? highlight_color : toolbar_text_color);
        if (debug)
            draw_text(runtime, "DEBUG", {72, 8}, 1,
                      on_toolbar(pointer_, settings_button_right, debug_button_right) ? highlight_color
                                                                                      : toolbar_text_color);
        return;
    }
    const auto positions = layout();
    const float actions = actions_y(positions);
    const Vec2 half{panel_width / 2, (actions + bottom_margin) / 2};
    const float size = scale(camera.viewport);
    const Vec2 origin = camera.viewport / 2 - half * size;
    runtime.box(camera.screen_to_world(camera.viewport / 2), half * size / camera.pixels_per_unit, 0,
                panel_color);
    const Vec2 point = (pointer_ - origin) / size;
    // Highlights use the same bands as picking. The full-width column includes its right
    // edge, as the click test does.
    auto line = [&](std::string_view text, float y, bool interactive, float x, float left = column_left,
                    float right = column_right) {
        const bool hovered = interactive && point.x >= left &&
                             (point.x < right || (right == column_right && point.x == right)) &&
                             in_band(point, y);
        draw_text(runtime, text, origin + Vec2{x, y} * size, size, hovered ? highlight_color : text_color);
    };
    auto centered = [&](std::string_view text, float y, bool interactive) {
        line(text, y, interactive, centered_x(text));
    };
    auto button = [&](const Button &b, float y) { line(b.text, y, true, b.text_x, b.left, b.right); };
    centered("SETTINGS", title_y, false);
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const float y = positions[i];
        if (std::holds_alternative<ZoomRow>(rows_[i])) {
            button(zoom_in, y);
            button(zoom_out, y);
        } else if (const auto *toggle = std::get_if<ToggleRow>(&rows_[i]))
            centered((*toggle->value ? "[X] " : "[ ] ") + toggle->label, y, true);
        else {
            const auto &section = std::get<HelpSection>(rows_[i]);
            centered((expanded_[i] ? "HIDE " : "SHOW ") + section.label, y, true);
            if (expanded_[i])
                for (std::size_t k = 0; k < section.lines.size(); ++k)
                    centered(section.lines[k], y + line_spacing * static_cast<float>(k + 1), false);
        }
    }
    button(launcher_button, actions);
    button(resume_button, actions);
    button(exit_button, actions);
    centered("CTRL  SETTINGS", actions + hint_spacing, false);
}

} // namespace kinetra
