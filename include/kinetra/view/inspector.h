#pragma once

#include <kinetra/simulation/math.h>

#include <charconv>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace kinetra {
/// One read-only diagnostic value, with units in the display text where applicable.
struct InspectorRow {
    /// Left-hand label and right-hand display value.
    std::string label, value;
};

/// A heading and its rows, in presentation order.
struct InspectorGroup {
    /// Optional heading above the rows.
    std::string heading;
    /// Rows in reading order.
    std::vector<InspectorRow> rows;
};

/// The details for one selected object at one instant. Owns all text and needs no runtime.
struct InspectorSnapshot {
    /// The text shown in the fixed title bar.
    std::string title;
    /// Groups in reading order.
    std::vector<InspectorGroup> groups;
};

/// Format a number as locale-independent inspector text. Precision is clamped to the
/// significant-digit range that a double can represent.
inline std::string inspector_number(double value, int digits = 5) {
    char buffer[64];
    const auto written = std::to_chars(buffer, buffer + sizeof buffer, value, std::chars_format::general,
                                       std::clamp(digits, 1, std::numeric_limits<double>::max_digits10));
    return std::string(buffer, written.ptr);
}

/// Format a two-dimensional value followed by its display units.
inline std::string inspector_vector(kinetra::Vec2 value, std::string_view unit) {
    auto text = inspector_number(value.x) + ", " + inspector_number(value.y);
    text += unit;
    return text;
}
} // namespace kinetra
