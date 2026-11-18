#pragma once

#include <array>
#include <kinetra/engine.h>
#include <string_view>

namespace kinetra {

/// Draw an uppercase bitmap label in logical screen pixels, independent of the world camera.
inline void draw_text(Runtime &runtime, std::string_view text, Vec2 origin, float scale, Color color) {
    // Five-bit rows keep menu/help text independent of external font assets.
    constexpr std::array<std::array<unsigned char, 7>, 26> letters{
        {{14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30}, {14, 17, 16, 16, 16, 17, 14},
         {30, 17, 17, 17, 17, 17, 30}, {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
         {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17}, {14, 4, 4, 4, 4, 4, 14},
         {7, 2, 2, 2, 18, 18, 12},     {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
         {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17}, {14, 17, 17, 17, 17, 17, 14},
         {30, 17, 17, 30, 16, 16, 16}, {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
         {15, 16, 16, 14, 1, 1, 30},   {31, 4, 4, 4, 4, 4, 4},       {17, 17, 17, 17, 17, 17, 14},
         {17, 17, 17, 17, 17, 10, 4},  {17, 17, 17, 21, 21, 21, 10}, {17, 17, 10, 4, 10, 17, 17},
         {17, 17, 10, 4, 4, 4, 4},     {31, 1, 2, 4, 8, 16, 31}}};
    constexpr std::array<std::array<unsigned char, 7>, 10> digits{{{14, 17, 19, 21, 25, 17, 14},
                                                                   {4, 12, 4, 4, 4, 4, 14},
                                                                   {14, 17, 1, 2, 4, 8, 31},
                                                                   {30, 1, 1, 14, 1, 1, 30},
                                                                   {2, 6, 10, 18, 31, 2, 2},
                                                                   {31, 16, 16, 30, 1, 1, 30},
                                                                   {14, 16, 16, 30, 17, 17, 14},
                                                                   {31, 1, 2, 4, 8, 8, 8},
                                                                   {14, 17, 17, 14, 17, 17, 14},
                                                                   {14, 17, 17, 15, 1, 1, 14}}};
    for (std::size_t column = 0; column < text.size(); ++column) {
        const char c = text[column];
        std::array<unsigned char, 7> glyph{};
        if (c >= 'A' && c <= 'Z')
            glyph = letters[c - 'A'];
        else if (c >= '0' && c <= '9')
            glyph = digits[c - '0'];
        else if (c == '/')
            glyph = {1, 1, 2, 4, 8, 16, 16};
        else if (c == '+')
            glyph = {0, 4, 4, 31, 4, 4, 0};
        else if (c == '[')
            glyph = {14, 8, 8, 8, 8, 8, 14};
        else if (c == ']')
            glyph = {14, 2, 2, 2, 2, 2, 14};
        else if (c == ':')
            glyph = {0, 4, 4, 0, 4, 4, 0};
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (glyph[y] & (1 << (4 - x))) {
                    const Vec2 center = origin + Vec2{(static_cast<float>(column * 6 + x) + 0.5f) * scale, (static_cast<float>(y) + 0.5f) * scale};
                    runtime.box(runtime.camera.screen_to_world(center),
                                Vec2{scale / 2, scale / 2} / runtime.camera.pixels_per_unit, 0, color);
                }
    }
}

} // namespace kinetra
