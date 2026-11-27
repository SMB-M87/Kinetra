#pragma once
// What a debug menu shows, as plain data. No drawing and no window: an application's logic
// layer builds these rows and tests them headless; DebugMenu (debug_view.h) draws them.

#include <kinetra/view/input.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace kinetra {
/// How a debug-menu row shows its state, and what its state means.
enum class DebugEntryKind {
    /// A command; it has no state.
    button,
    /// An on/off setting, drawn as a check box; `active` says whether it is on.
    check,
    /// One value of an open list, drawn as a radio mark; `active` says whether it is the
    /// current value.
    choice,
    /// A heading that opens and closes the rows below it; `active` says whether it is open.
    group,
};

/// One row of a debug menu.
///
/// The functions borrow whatever made the entry: call them only while that object is alive.
/// Build the rows anew after any of them ran, and for every update and every draw, because
/// open sections, open lists and current settings decide which rows exist.
struct DebugEntry {
    /// Row text without indentation or state mark.
    std::string label;
    /// Runs when the row is clicked outside its buttons. Empty on a row that only shows
    /// something, such as a number that changes with `decrease` and `increase`.
    std::function<void()> action{};
    /// How the row shows its state.
    DebugEntryKind kind{DebugEntryKind::button};
    /// Current state for every kind but `button`; empty counts as false.
    std::function<bool()> active{};
    /// Current value, shown right-aligned: a number or the name of a choice. May be empty.
    std::function<std::string()> value{};
    /// Run by the row's `[-]` and `[+]` buttons, which exist only when these are set. Either
    /// may be set alone.
    std::function<void()> decrease{}, increase{};
    /// Applies a whole number typed over the row: digits typed while the pointer is on the
    /// row replace its value on screen, and Enter passes them here (see DebugMenu). The
    /// function clamps or rejects a number outside its own limits. Empty on a row that takes
    /// no typed number.
    std::function<void(std::uint64_t)> set{};
    /// Nesting level: 0 for a section heading and for rows outside sections, 1 for the rows
    /// of a section, 2 for the values of an open list and the parts of a row.
    int depth{};
    /// Color of a small square at the right edge, as a legend for what the row draws.
    std::optional<Color> swatch{};
    /// Draws a line across the menu above this row, to set commands apart from sections.
    bool rule_above{};
};

/// Remembers which section and which list of a debug menu are open, and makes their heading
/// rows.
///
/// At most one section is open: opening one closes the other, and a click on the open one
/// closes it. A list belongs to the section it was opened in and closes with it. Sections
/// and lists are known by their labels, which must be unique among their kind. Not copyable,
/// because the rows it makes refer to it.
class DebugFolds {
  public:
    /// Starts with the section `open_section` open; empty for none.
    explicit DebugFolds(std::string open_section = {}) : section_(std::move(open_section)) {}
    DebugFolds(const DebugFolds &) = delete;
    DebugFolds &operator=(const DebugFolds &) = delete;

    /// The heading row of section `label`, at depth 0.
    DebugEntry section(std::string label) {
        return {.label = label,
                .action =
                    [this, label] {
                        section_ = section_ == label ? std::string() : label;
                        list_.clear();
                    },
                .kind = DebugEntryKind::group,
                .active = [this, label] { return section_ == label; }};
    }
    /// Whether section `label` is open, so its rows belong in the menu.
    bool section_open(std::string_view label) const { return section_ == label; }
    /// The heading row of the list `label` at `depth`, showing `value`, the name of the
    /// current choice. A click opens the list or closes it.
    DebugEntry list(std::string label, std::function<std::string()> value, int depth = 1) {
        return {.label = label,
                .action = [this, label] { list_ = list_ == label ? std::string() : label; },
                .kind = DebugEntryKind::group,
                .active = [this, label] { return list_ == label; },
                .value = std::move(value),
                .depth = depth};
    }
    /// Whether the list `label` is open, so its choices belong in the menu.
    bool list_open(std::string_view label) const { return list_ == label; }
    /// Closes the open list; a choice row calls this after it applied its value.
    void close_list() { list_.clear(); }

  private:
    std::string section_, list_;
};

/// `value` for a menu row: three significant digits, and a whole number from 1000 on, so
/// it stays short enough to end before the row's buttons.
inline std::string debug_number_text(float value) {
    char text[32];
    if (std::abs(value) >= 1000)
        std::snprintf(text, sizeof text, "%.0f", static_cast<double>(value));
    else
        std::snprintf(text, sizeof text, "%.3g", static_cast<double>(value));
    return text;
}

/// A check row over `flag`, which the row borrows: a click flips it.
inline DebugEntry debug_check(std::string label, bool &flag, int depth = 1) {
    return {.label = std::move(label),
            .action = [&flag] { flag = !flag; },
            .kind = DebugEntryKind::check,
            .active = [&flag] { return flag; },
            .depth = depth};
}

/// How the buttons of a number row change its value.
struct DebugStep {
    /// Inclusive limits; a step never leaves them.
    float minimum, maximum;
    /// Size of one step: a factor above 1 when `geometric`, otherwise an amount above 0.
    float amount;
    /// Whether a step multiplies or divides by `amount` instead of adding or subtracting it.
    bool geometric{};
};

/// A number row over `value`, which the row borrows: it shows the value and its `[-]` and
/// `[+]` buttons step it within the limits of `step`. Throws std::invalid_argument for
/// limits that are not finite and ordered, or a step that would not change the value.
inline DebugEntry debug_number(std::string label, float &value, DebugStep step, int depth = 1) {
    if (!std::isfinite(step.minimum) || !std::isfinite(step.maximum) || step.minimum > step.maximum ||
        !std::isfinite(step.amount) || (step.geometric ? step.amount <= 1 : step.amount <= 0))
        throw std::invalid_argument("Invalid debug number step");
    const auto stepped = [step](float current, bool up) {
        const float next = step.geometric ? (up ? current * step.amount : current / step.amount)
                                          : (up ? current + step.amount : current - step.amount);
        // Rounded to four decimals, so a step up and a step down return to the same value
        // instead of leaving the last bits of a float product behind.
        return std::clamp(std::round(next * 10000) / 10000, step.minimum, step.maximum);
    };
    return {.label = std::move(label),
            .value = [&value] { return debug_number_text(value); },
            .decrease = [&value, stepped] { value = stepped(value, false); },
            .increase = [&value, stepped] { value = stepped(value, true); },
            .depth = depth};
}

/// A number row over the whole number `value`, which the row borrows: its buttons add or
/// subtract one within [`minimum`, `maximum`]. Throws std::invalid_argument when the limits
/// are not ordered.
inline DebugEntry debug_count(std::string label, int &value, int minimum, int maximum, int depth = 1) {
    if (minimum > maximum)
        throw std::invalid_argument("Invalid debug count limits");
    return {.label = std::move(label),
            .value = [&value] { return std::to_string(value); },
            .decrease = [&value, minimum, maximum] { value = std::clamp(value - 1, minimum, maximum); },
            .increase = [&value, minimum, maximum] { value = std::clamp(value + 1, minimum, maximum); },
            .depth = depth};
}
} // namespace kinetra
