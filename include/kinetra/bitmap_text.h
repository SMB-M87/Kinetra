#pragma once

#include <array>
#include <kinetra/engine.h>
#include <string_view>

namespace kinetra {

/// One character of the built-in bitmap font: seven rows of five pixels, top row first.
/// Bit 4 of each row is the left column, bit 0 the right column.
using Glyph = std::array<unsigned char, 7>;

/// Returns the bitmap for `c`.
///
/// Covers `A`–`Z`, `0`–`9` and `/ + [ ] : - _ . , *`. Lowercase letters use the uppercase
/// glyph, so lowercase text stays readable without a second alphabet. Every other
/// character, including space, is blank. Pure and usable without a Runtime.
constexpr Glyph glyph(char c) {
    // Five-bit rows keep menu/help text independent of external font assets.
    constexpr std::array<Glyph, 26> letters{
        {{14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30}, {14, 17, 16, 16, 16, 17, 14},
         {30, 17, 17, 17, 17, 17, 30}, {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
         {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17}, {14, 4, 4, 4, 4, 4, 14},
         {7, 2, 2, 2, 18, 18, 12},     {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
         {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17}, {14, 17, 17, 17, 17, 17, 14},
         {30, 17, 17, 30, 16, 16, 16}, {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
         {15, 16, 16, 14, 1, 1, 30},   {31, 4, 4, 4, 4, 4, 4},       {17, 17, 17, 17, 17, 17, 14},
         {17, 17, 17, 17, 17, 10, 4},  {17, 17, 17, 21, 21, 21, 10}, {17, 17, 10, 4, 10, 17, 17},
         {17, 17, 10, 4, 4, 4, 4},     {31, 1, 2, 4, 8, 16, 31}}};
    constexpr std::array<Glyph, 10> digits{{{14, 17, 19, 21, 25, 17, 14},
                                            {4, 12, 4, 4, 4, 4, 14},
                                            {14, 17, 1, 2, 4, 8, 31},
                                            {30, 1, 1, 14, 1, 1, 30},
                                            {2, 6, 10, 18, 31, 2, 2},
                                            {31, 16, 16, 30, 1, 1, 30},
                                            {14, 16, 16, 30, 17, 17, 14},
                                            {31, 1, 2, 4, 8, 8, 8},
                                            {14, 17, 17, 14, 17, 17, 14},
                                            {14, 17, 17, 15, 1, 1, 14}}};
    if (c >= 'A' && c <= 'Z')
        return letters[static_cast<std::size_t>(c - 'A')];
    if (c >= 'a' && c <= 'z')
        return letters[static_cast<std::size_t>(c - 'a')];
    if (c >= '0' && c <= '9')
        return digits[static_cast<std::size_t>(c - '0')];
    switch (c) {
    case '/':
        return {1, 1, 2, 4, 8, 16, 16};
    case '+':
        return {0, 4, 4, 31, 4, 4, 0};
    case '[':
        return {14, 8, 8, 8, 8, 8, 14};
    case ']':
        return {14, 2, 2, 2, 2, 2, 14};
    case ':':
        return {0, 4, 4, 0, 4, 4, 0};
    case '-':
        return {0, 0, 0, 31, 0, 0, 0};
    case '_':
        return {0, 0, 0, 0, 0, 0, 31};
    case '.':
        return {0, 0, 0, 0, 0, 0, 4};
    case ',':
        return {0, 0, 0, 0, 0, 4, 8};
    case '*':
        return {0, 4, 21, 14, 21, 4, 0};
    default:
        return {};
    }
}

/// Draws `text` with the built-in bitmap font in logical screen pixels, independent of the
/// world camera.
///
/// `origin` is the top-left corner of the first character in screen pixels; each character
/// cell is 6 × 7 font pixels of `scale` screen pixels. Characters are mapped by glyph().
inline void draw_text(Runtime &runtime, std::string_view text, Vec2 origin, float scale, Color color) {
    for (std::size_t column = 0; column < text.size(); ++column) {
        const Glyph rows = glyph(text[column]);
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (rows[static_cast<std::size_t>(y)] & (1 << (4 - x))) {
                    const Vec2 center =
                        origin +
                        Vec2{(static_cast<float>(column * 6 + static_cast<std::size_t>(x)) + 0.5f) * scale,
                             (static_cast<float>(y) + 0.5f) * scale};
                    runtime.box(runtime.camera.screen_to_world(center),
                                Vec2{scale / 2, scale / 2} / runtime.camera.pixels_per_unit, 0, color);
                }
    }
}

} // namespace kinetra
