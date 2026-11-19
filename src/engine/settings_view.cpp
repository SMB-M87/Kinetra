#include <kinetra/bitmap_text.h>
#include <kinetra/overlay_style.h>
#include <kinetra/settings_view.h>

#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace kinetra {
namespace {
// Layout in logical panel pixels. A row's position is the top edge of its text; the row is
// clickable, and lights up, in a band around it.
constexpr float title_height = 22;
constexpr float first_row_y = 33;
constexpr float row_spacing = 18;
constexpr float band_above = 5;
constexpr float band_below = row_spacing - band_above;
// Extra space before the first help section, and between a section's last line and the
// next row.
constexpr float section_gap = 4;
constexpr float line_spacing = 12;
constexpr float lines_gap = 4;
// From where the next row would be to the text of the action buttons, with the rule between.
constexpr float actions_gap = 12;
constexpr float rule_above_actions = 11;
constexpr float bottom_margin = 22;
constexpr float button_height = 18;
// Columns: the state mark, and the label with the help lines below it.
constexpr float mark_x = 12;
constexpr float label_x = 24;
// The zoom buttons, like the steppers of a debug-menu row.
constexpr float stepper_width = 14;

/// A button of the action row: clickable and drawn `width` wide around `center`.
struct Button {
    std::string_view text;
    float center, width;
    constexpr float left() const { return center - width / 2; }
    constexpr float right() const { return center + width / 2; }
};
constexpr Button launcher_button{"LAUNCHER", SettingsOverlay::launcher_x, 72};
constexpr Button resume_button{"RESUME", SettingsOverlay::resume_x, 56};
constexpr Button exit_button{"EXIT", SettingsOverlay::exit_x, 40};

// Closed-state toolbar, in screen pixels at the top-left corner of the window. A button is
// clickable in its whole cell and drawn `toolbar_inset` inside it.
constexpr float toolbar_height = 24;
constexpr float settings_button_right = 64;
constexpr float debug_button_right = 112;
constexpr float toolbar_inset = 3;
constexpr Vec2 debug_menu_origin{8, 32};

bool in_band(Vec2 point, float y) {
    return point.y >= y - band_above && point.y < y + band_below;
}
bool on_toolbar(Vec2 mouse, float left, float right) {
    return mouse.x >= left && mouse.x < right && mouse.y >= 0 && mouse.y < toolbar_height;
}
bool is_section(const SettingsRow &row) {
    return std::holds_alternative<HelpSection>(row);
}
/// Which zoom button panel x coordinate `x` is on: -1 for out, 1 for in, 0 for neither.
/// Each zone is its button and half the gap to the other one.
int zoom_button_at(float x) {
    const float middle = (SettingsOverlay::zoom_out_x + SettingsOverlay::zoom_in_x) / 2;
    if (x < SettingsOverlay::zoom_out_x - stepper_width / 2 ||
        x > SettingsOverlay::zoom_in_x + stepper_width / 2)
        return 0;
    return x < middle ? -1 : 1;
}
} // namespace

SettingsOverlay::SettingsOverlay(std::vector<SettingsRow> rows, bool *debug_mode)
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
    // 228 x 315 fits the tallest Lab panel, with both help sections open, and a margin. The
    // scale never depends on the expanded state, so opening a section does not resize the
    // panel.
    return std::min({2.f, viewport.x / 228.f, viewport.y / 315.f});
}

std::vector<float> SettingsOverlay::layout() const {
    std::vector<float> positions;
    positions.reserve(rows_.size() + 1);
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
        if (section && expanded_[i] && !section->lines.empty())
            y += line_spacing * static_cast<float>(section->lines.size()) + lines_gap;
    }
    // One past the last row: where the next row would be.
    positions.push_back(y);
    return positions;
}

float SettingsOverlay::actions_y() const {
    return layout().back() + actions_gap;
}

Vec2 SettingsOverlay::half_extent() const {
    return {panel_width / 2, (actions_y() + bottom_margin) / 2};
}

float SettingsOverlay::row_y(std::size_t index) const {
    if (index >= rows_.size())
        throw std::out_of_range("Settings row out of range");
    return layout()[index];
}

bool SettingsOverlay::expanded(std::size_t index) const {
    return expanded_.at(index);
}

bool SettingsOverlay::update(const Input &input, Camera2D &camera) {
    pointer_ = input.mouse;
    const bool was_visible = visible_;
    // Ctrl acts when it is released, so Ctrl followed by Shift is still the debug shortcut
    // and never opens this panel first. A press whose key is not held, as scripted input
    // sends it, is a complete tap.
    const bool shift = input.down(Key::shift) || input.hit(Key::shift);
    if (input.hit(Key::control))
        control_armed_ = !shift;
    else if (shift)
        control_armed_ = false;
    const bool tapped = control_armed_ && !input.down(Key::control);
    if (tapped)
        control_armed_ = false;
    if (tapped || (!visible_ && input.clicked && on_toolbar(input.mouse, 0, settings_button_right))) {
        visible_ = !visible_;
        return modal_ = was_visible || visible_;
    }
    modal_ = was_visible || visible_;
    if (!visible_ || !input.clicked)
        return modal_;
    // Positions are taken before this click changes any section, as drawn last frame.
    const auto positions = layout();
    const float actions = positions.back() + actions_gap;
    const Vec2 half{panel_width / 2, (actions + bottom_margin) / 2};
    const Vec2 point = (input.mouse - camera.viewport / 2) / scale(camera.viewport) + half;
    if (point.x < 0 || point.x > panel_width)
        return true;
    auto on = [&](const Button &button) { return point.x >= button.left() && point.x < button.right(); };
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
            if (const int direction = zoom_button_at(point.x))
                zoom(camera, static_cast<float>(direction), limits);
        } else if (const auto *toggle = std::get_if<ToggleRow>(&rows_[i]))
            *toggle->value = !*toggle->value;
        else
            expanded_[i] = !expanded_[i];
        break;
    }
    return true;
}

bool SettingsOverlay::update_debug(const Input &input, Vec2 viewport, DebugMenu &menu,
                                   std::span<const DebugEntry> entries) {
    menu.screen_origin = debug_menu_origin;
    debug_open_ = false;
    if (modal_ || !debug_mode_) {
        menu.visible = false;
        return false;
    }
    if (debug_chord(input)) {
        // The shortcut is also the way into the debug mode, so it never needs the panel.
        menu.visible = !*debug_mode_ || !menu.visible;
        *debug_mode_ = true;
    }
    if (!*debug_mode_) {
        menu.visible = false;
        return false;
    }
    bool captured = menu.update(input, viewport, entries);
    if (on_toolbar(input.mouse, settings_button_right, debug_button_right)) {
        if (input.clicked)
            menu.visible = !menu.visible;
        captured = true;
    }
    debug_open_ = menu.visible;
    return captured;
}

void SettingsOverlay::draw(Runtime &runtime) const {
    const Camera2D &camera = runtime.camera;
    if (!visible_) {
        const Vec2 size{0, toolbar_height - 2 * toolbar_inset};
        auto button = [&](std::string_view text, float left, float right, bool lit) {
            overlay::draw_text_button(runtime, {left + toolbar_inset, toolbar_inset},
                                      {right - left - 2 * toolbar_inset, size.y}, text, 1, lit);
        };
        button("SETTINGS", 0, settings_button_right, on_toolbar(pointer_, 0, settings_button_right));
        if (debug_mode_ && *debug_mode_)
            button("DEBUG", settings_button_right, debug_button_right,
                   debug_open_ || on_toolbar(pointer_, settings_button_right, debug_button_right));
        return;
    }
    const auto positions = layout();
    const float actions = positions.back() + actions_gap;
    const Vec2 half{panel_width / 2, (actions + bottom_margin) / 2};
    const float size = scale(camera.viewport);
    const Vec2 origin = camera.viewport / 2 - half * size;
    // Screen position of a point given in panel pixels, on a whole pixel.
    const auto at = [&](float x, float y) { return overlay::snapped(origin + Vec2{x, y} * size); };
    overlay::draw_panel(runtime, origin, half * (2 * size), title_height * size, "SETTINGS", "CTRL", size);
    const Vec2 point = (pointer_ - origin) / size;
    const bool inside = point.x >= 0 && point.x <= panel_width;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const float y = positions[i];
        const bool on_row = inside && in_band(point, y);
        const bool zoom_row = std::holds_alternative<ZoomRow>(rows_[i]);
        // A zoom row is two buttons; the others are clickable as a whole.
        const bool lit = on_row && !zoom_row;
        if (lit)
            overlay::draw_rectangle(runtime, at(1, y - band_above), Vec2{panel_width - 2, row_spacing} * size,
                                    overlay::band_color);
        const Color color = lit ? overlay::bright_color : overlay::text_color;
        if (zoom_row) {
            draw_text(runtime, "ZOOM", at(label_x, y), size, color);
            const int hovered = on_row ? zoom_button_at(point.x) : 0;
            const Vec2 button{stepper_width * size, (row_spacing - 4) * size};
            overlay::draw_sign_button(runtime, at(zoom_out_x - stepper_width / 2, y - band_above + 2), button,
                                      '-', size, hovered < 0);
            overlay::draw_sign_button(runtime, at(zoom_in_x - stepper_width / 2, y - band_above + 2), button,
                                      '+', size, hovered > 0);
        } else if (const auto *toggle = std::get_if<ToggleRow>(&rows_[i])) {
            const bool on = *toggle->value;
            draw_glyph(runtime, on ? overlay::check_on : overlay::check_off, at(mark_x, y), size,
                       on ? overlay::on_color : overlay::dim_color);
            draw_text(runtime, toggle->label, at(label_x, y), size, color);
        } else {
            const auto &section = std::get<HelpSection>(rows_[i]);
            draw_glyph(runtime, expanded_[i] ? overlay::arrow_open : overlay::arrow_closed, at(mark_x, y),
                       size, overlay::accent_color);
            draw_text(runtime, section.label, at(label_x, y), size, color);
            if (expanded_[i])
                for (std::size_t k = 0; k < section.lines.size(); ++k)
                    draw_text(runtime, section.lines[k],
                              at(label_x, y + row_spacing + line_spacing * static_cast<float>(k)), size,
                              overlay::dim_color);
        }
    }
    overlay::draw_rectangle(runtime, at(mark_x, actions - rule_above_actions),
                            {(panel_width - 2 * mark_x) * size, std::max(1.f, size)}, overlay::rule_color);
    const float button_top = actions - (button_height - 7) / 2;
    for (const auto &button : {launcher_button, resume_button, exit_button})
        overlay::draw_text_button(
            runtime, at(button.left(), button_top), Vec2{button.width, button_height} * size, button.text,
            size, inside && in_band(point, actions) && point.x >= button.left() && point.x < button.right());
}

} // namespace kinetra
