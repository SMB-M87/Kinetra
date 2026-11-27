#pragma once
// The style of the text an application lays over its view: statistics lines at the bottom of
// the window and an error message directly above them. Every application and the launch menu
// draw with these, so an error reads the same everywhere.

#include <kinetra/runtime/bitmap_text.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kinetra::hud {
/// Screen pixels per font pixel of HUD text.
inline constexpr float text_scale = 1.5f;
/// Height of one HUD line in screen pixels: the 7-pixel glyph at text_scale plus a gap.
inline constexpr float line_height = 20;
/// Distance of HUD text from the left and right edges of the viewport, in screen pixels.
inline constexpr float margin = 10;
/// Text of the statistics lines.
inline constexpr Color statistics_color{1, 1, 0};
/// Text of an error message: a clear red that stands out from the yellow statistics.
inline constexpr Color error_color{1, 0.25f, 0.25f};
/// Marks the end of an error message that was cut to its maximum number of lines.
inline constexpr std::string_view cut_mark = "...";

/// Characters of HUD text that fit on one line across a viewport `width` screen pixels wide,
/// with a margin on each side; at least one, so wrapping always moves forward.
constexpr std::size_t columns(float width) {
    const float available = (width - 2 * margin) / (6 * text_scale);
    return available < 1 ? 1 : static_cast<std::size_t>(available);
}

/// Top of HUD line `line` in screen pixels, counted from the bottom of `viewport`: line 1 is
/// the lowest.
constexpr float line_top(Vec2 viewport, int line) {
    return viewport.y - static_cast<float>(line) * line_height;
}

/// Whole HUD lines that fit between screen y `top` and the bottom of `viewport`; zero when
/// `top` is at or below the bottom.
constexpr int lines_below(Vec2 viewport, float top) {
    const float space = (viewport.y - top) / line_height;
    return space < 1 ? 0 : static_cast<int>(space);
}

/// `text` split into lines of at most `columns` characters. A line breaks at the last space
/// that fits and drops the spaces on both sides of the break; a word longer than a line, or
/// a break before the first word, cuts at the line's end. Empty
/// text gives no lines. The views point into `text`. Throws std::invalid_argument for zero
/// columns.
inline std::vector<std::string_view> wrap(std::string_view text, std::size_t columns) {
    if (columns == 0)
        throw std::invalid_argument("HUD text needs at least one column");
    const auto trimmed = [](std::string_view part) {
        while (!part.empty() && part.back() == ' ')
            part.remove_suffix(1);
        return part;
    };
    std::vector<std::string_view> lines;
    while (!text.empty()) {
        std::string_view line;
        std::size_t used = columns;
        if (text.size() <= columns) {
            line = trimmed(text);
            used = text.size();
        } else if (const auto space = text.rfind(' ', columns); space != std::string_view::npos) {
            // Prefer the last space within reach, so words stay whole.
            line = trimmed(text.substr(0, space));
            used = space;
        }
        // No space, or only spaces before it: cut the line at its end.
        if (line.empty() && used != text.size()) {
            line = text.substr(0, columns);
            used = columns;
        }
        if (!line.empty())
            lines.push_back(line);
        text.remove_prefix(used);
        // Every space at a break goes, so the next line starts with its word.
        while (!text.empty() && text.front() == ' ')
            text.remove_prefix(1);
    }
    return lines;
}

/// The lines of an error `message` as draw_error() shows them: wrapped to `columns`, and at most
/// `max_lines`, where a cut message ends its last line with cut_mark, or as much of it as fits.
/// No line is longer than `columns`. Zero `max_lines` gives no lines. Throws std::invalid_argument for zero
/// columns.
inline std::vector<std::string> error_lines(std::string_view message, std::size_t columns, int max_lines) {
    const auto wrapped = wrap(message, columns);
    const auto shown = std::min(wrapped.size(), static_cast<std::size_t>(std::max(0, max_lines)));
    std::vector<std::string> lines(wrapped.begin(), wrapped.begin() + static_cast<std::ptrdiff_t>(shown));
    if (shown < wrapped.size() && shown > 0) {
        auto &last = lines.back();
        // A line narrower than the mark keeps as much of the mark as fits, never more.
        const auto mark = cut_mark.substr(0, std::min(cut_mark.size(), columns));
        last = last.substr(0, std::min(last.size(), columns - mark.size())) + std::string(mark);
    }
    return lines;
}

/// Draws `text` on HUD line `line` (1 is the lowest) at the left margin, in logical screen
/// pixels, inside a frame the caller began.
inline void draw_line(Runtime &runtime, std::string_view text, int line, Color color) {
    draw_text(runtime, text, {margin, line_top(runtime.camera.viewport, line)}, text_scale, color);
}

/// Draws the error `message` in error_color, wrapped to the viewport width, with its last line
/// on HUD line `line` and the lines before it above, at most `max_lines` (see error_lines()).
/// Returns the number of lines drawn, so the caller can place its next text above them; an
/// empty message draws nothing and returns zero.
inline int draw_error(Runtime &runtime, std::string_view message, int line, int max_lines) {
    const auto lines = error_lines(message, columns(runtime.camera.viewport.x), max_lines);
    const auto count = static_cast<int>(lines.size());
    for (int i = 0; i < count; ++i)
        draw_line(runtime, lines[static_cast<std::size_t>(i)], line + count - 1 - i, error_color);
    return count;
}
} // namespace kinetra::hud
