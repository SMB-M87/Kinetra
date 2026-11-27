#pragma once
// An error message in a panel over a screen that has no HUD lines to spare, such as a launch
// menu: the text in the HUD error style (hud_text.h), as many lines as it needs up to the
// whole window, and a close button like the inspector's.

#include <kinetra/view/input.h>

#include <string>
#include <string_view>
#include <vector>

namespace kinetra {
class Runtime;

/// What the error panel did with this frame's pointer.
struct ErrorPanelInput {
    /// The pointer is over the panel. The caller keeps the pointer from what lies below.
    bool captured{};
    /// The close button was clicked. The caller removes the message.
    bool close{};
};

namespace error_panel {
/// The panel for `message` in a viewport of logical window pixels: as wide as the viewport
/// less the HUD margin on each side, at the bottom, and as high as its lines need, up to the
/// viewport less the margins. A zero-sized box at the bottom center for an empty message, or
/// for a viewport too small for a title bar and one line.
Aabb bounds(std::string_view message, Vec2 viewport);
/// The close button of the panel for `message`, at the right of its title bar; a zero-sized
/// box when the panel has none.
Aabb close_bounds(std::string_view message, Vec2 viewport);
/// The lines of `message` the panel shows: wrapped to its width in the HUD error style, and
/// cut with hud::cut_mark when even the whole viewport cannot hold them. None for an empty
/// message or a viewport the panel does not fit.
std::vector<std::string> lines(std::string_view message, Vec2 viewport);
/// Reads this frame's pointer against the panel for `message`.
ErrorPanelInput update(const Input &input, std::string_view message, Vec2 viewport);
/// Draws the panel for `message` inside an active runtime frame, with the close button lit
/// while `pointer` (logical window pixels) is on it. Draws nothing for an empty message.
void draw(Runtime &runtime, std::string_view message, Vec2 pointer);
} // namespace error_panel
} // namespace kinetra
