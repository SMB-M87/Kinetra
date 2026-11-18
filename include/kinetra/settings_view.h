#pragma once

#include <kinetra/camera.h>
#include <kinetra/debug_view.h>
#include <kinetra/session.h>

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace kinetra {

/// ZOOM IN / ZOOM OUT button pair. The limits are read on each click, so they may depend
/// on application state such as a debug mode.
struct ZoomRow {
    std::function<ZoomLimits()> limits;
};

/// "[X] LABEL" check box over a borrowed flag. The flag must outlive the overlay's
/// update() and draw() calls.
struct ToggleRow {
    std::string label;
    bool *value{};
};

/// "SHOW LABEL" / "HIDE LABEL" button followed, while expanded, by read-only help lines.
/// Each section starts collapsed and keeps its own expanded state.
struct HelpSection {
    std::string label;
    std::vector<std::string> lines;
};

/// One application-supplied entry of the settings panel, in display order.
using SettingsRow = std::variant<ZoomRow, ToggleRow, HelpSection>;

/// Modal settings panel that applications fill with their own rows.
///
/// Kinetra owns the title, the closed-state toolbar (SETTINGS, and DEBUG while the debug
/// mode is on), the LAUNCHER / RESUME / EXIT action row, the hint and the layout.
/// Applications own the rows. Layout is in logical panel pixels and is shared by picking
/// and drawing, so what is drawn is what is clicked. While open, the panel captures all
/// input, including the click that closes it. See ADR 0013.
class SettingsOverlay {
  public:
    /// Width of the panel in logical panel pixels.
    static constexpr float panel_width = 184;

    /// Takes the rows in display order. `debug_mode`, if given, is borrowed, must outlive
    /// the overlay, and shows the DEBUG toolbar button while true.
    /// Throws std::invalid_argument when a zoom or toggle row follows a help section, when
    /// a toggle row has no flag, or when a zoom row has no limits.
    explicit SettingsOverlay(std::vector<SettingsRow> rows, const bool *debug_mode = nullptr);

    /// Applies one frame of input. Ctrl or the SETTINGS toolbar button opens and closes
    /// the panel; clicks inside the open panel apply rows and actions. Returns true while
    /// input is captured: when the panel is open, or was open at the start of the frame.
    bool update(const Input &input, Camera2D &camera);

    /// Handles the DEBUG toolbar button and places `menu` below the toolbar. Returns true
    /// when the pointer is on the button or the debug menu captured the input.
    bool update_debug(const Input &input, Vec2 viewport, DebugMenu &menu,
                      std::span<DebugOption> options) const;

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
    /// Vertical position of row `index` in panel pixels (the text's top edge).
    /// Throws std::out_of_range for an invalid index.
    float row_y(std::size_t index) const;
    /// Vertical position of the LAUNCHER / RESUME / EXIT row in panel pixels.
    float actions_y() const;
    /// Whether the help section at row `index` is expanded; false for other rows.
    /// Throws std::out_of_range for an invalid index.
    bool expanded(std::size_t index) const;

  private:
    std::vector<SettingsRow> rows_;
    std::vector<bool> expanded_;
    const bool *debug_mode_;
    bool visible_{};
    SessionRequest request_{SessionRequest::none};
    Vec2 pointer_{-1, -1};

    std::vector<float> layout() const;
    float actions_y(const std::vector<float> &positions) const;
};

} // namespace kinetra
