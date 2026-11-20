#pragma once

#include <kinetra/runtime/debug_view.h>
#include <kinetra/runtime/session.h>
#include <kinetra/view/camera.h>

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace kinetra {

/// "ZOOM" with a `[-]` and a `[+]` button. The limits are read on each click, so they may
/// depend on application state such as a debug mode.
struct ZoomRow {
    std::function<ZoomLimits()> limits;
};

/// Check box and label over a borrowed flag. The flag must outlive the overlay's update()
/// and draw() calls.
struct ToggleRow {
    std::string label;
    bool *value{};
};

/// A heading that opens and closes read-only help lines below it. Each section starts
/// collapsed and keeps its own expanded state.
struct HelpSection {
    std::string label;
    std::vector<std::string> lines;
};

/// One application-supplied entry of the settings panel, in display order.
using SettingsRow = std::variant<ZoomRow, ToggleRow, HelpSection>;

/// Modal settings panel that applications fill with their own rows.
///
/// Kinetra owns the title bar, the closed-state toolbar (SETTINGS, and DEBUG while the debug
/// mode is on), the LAUNCHER / RESUME / EXIT action row, the layout and the two shortcuts:
/// Ctrl for this panel and Shift+Ctrl for the debug menu. Applications own the rows. Layout
/// is in logical panel pixels and is shared by picking and drawing, so what is drawn is
/// what is clicked. While open, the panel captures all input, including the click that
/// closes it. See ADR 0013.
class SettingsOverlay {
  public:
    /// Width of the panel in logical panel pixels.
    static constexpr float panel_width = 208;
    /// Centers of the LAUNCHER, RESUME and EXIT buttons on the action row, in panel pixels.
    static constexpr float launcher_x = 48, resume_x = 120, exit_x = 176;
    /// Centers of the `[-]` and `[+]` buttons of a zoom row, in panel pixels.
    static constexpr float zoom_out_x = 171, zoom_in_x = 187;

    /// Takes the rows in display order. `debug_mode`, if given, is borrowed and must outlive
    /// the overlay: the DEBUG toolbar button shows while it is true, and update_debug() sets
    /// it when the debug shortcut is used.
    /// Throws std::invalid_argument when a zoom or toggle row follows a help section, when
    /// a toggle row has no flag, or when a zoom row has no limits.
    explicit SettingsOverlay(std::vector<SettingsRow> rows, bool *debug_mode = nullptr);

    /// Applies one frame of input. The SETTINGS toolbar button or Ctrl opens and closes the
    /// panel; Ctrl acts when it is released, and not at all when Shift was down while it
    /// was held, because Shift+Ctrl is the debug shortcut, or when the window lost focus
    /// while it was held. Clicks inside the open panel
    /// apply rows and actions. Returns true while input is captured: when the panel is
    /// open, or was open at the start of the frame.
    bool update(const Input &input, Camera2D &camera);

    /// Shows, hides and updates the debug menu; call it every frame after update(), with
    /// the rows `menu` is drawn with. Places `menu` below the toolbar. Shift+Ctrl
    /// (debug_chord) switches the debug mode on and shows the menu, or hides a showing menu;
    /// the DEBUG toolbar button shows and hides it. While the panel captures input or the
    /// debug mode is off, the menu is hidden and nothing is captured; so too for an overlay
    /// without a debug flag. Returns true when the pointer is on the button or on the menu,
    /// so the caller must ignore that pointer input.
    bool update_debug(const Input &input, Vec2 viewport, DebugMenu &menu,
                      std::span<const DebugEntry> entries);

    /// Draws the toolbar while closed, or the panel while open, in screen space.
    void draw(Runtime &runtime) const;

    /// Whether the panel is open.
    bool visible() const { return visible_; }

    /// `menu` after LAUNCHER, `exit` after EXIT, `none` otherwise. Requests are not reset:
    /// the host acts on them once (see SessionRequest).
    SessionRequest request() const { return request_; }

    /// Screen pixels per panel pixel for a viewport in logical window pixels.
    static float scale(Vec2 viewport);
    /// Half size of the panel in panel pixels; it grows when help sections expand.
    Vec2 half_extent() const;
    /// Vertical position of row `index` in panel pixels: the top edge of its text. The row
    /// is clickable across the panel from 5 pixels above to 13 below.
    /// Throws std::out_of_range for an invalid index.
    float row_y(std::size_t index) const;
    /// Vertical position of the LAUNCHER / RESUME / EXIT row in panel pixels: the top edge
    /// of the button texts.
    float actions_y() const;
    /// Whether the help section at row `index` is expanded; false for other rows.
    /// Throws std::out_of_range for an invalid index.
    bool expanded(std::size_t index) const;

  private:
    std::vector<SettingsRow> rows_;
    std::vector<bool> expanded_;
    bool *debug_mode_;
    bool visible_{};
    /// What the last update() returned: the panel captures this frame's input.
    bool modal_{};
    /// Ctrl went down without Shift and has not been released yet.
    bool control_armed_{};
    /// Whether the debug menu showed after the last update_debug(), to light its button.
    bool debug_open_{};
    SessionRequest request_{SessionRequest::none};
    Vec2 pointer_{-1, -1};

    /// The position of every row, followed by where one more row would be.
    std::vector<float> layout() const;
};

} // namespace kinetra
