// Window-free tests for the header-only presentation helpers: the bitmap glyph table
// (bitmap_text.h), camera zoom stepping (camera.h) and the rate counter (timing.h).
#include <kinetra/bitmap_text.h>
#include <kinetra/camera.h>
#include <kinetra/timing.h>

#include <doctest/doctest.h>

#include <stdexcept>

using kinetra::Glyph;
using kinetra::glyph;

TEST_CASE("punctuation glyphs use the pixels i4sim drew by hand") {
    CHECK(glyph('-') == Glyph{0, 0, 0, 31, 0, 0, 0});
    CHECK(glyph('_') == Glyph{0, 0, 0, 0, 0, 0, 31});
    CHECK(glyph('.') == Glyph{0, 0, 0, 0, 0, 0, 4});
    CHECK(glyph(',') == Glyph{0, 0, 0, 0, 0, 4, 8});
    CHECK(glyph('*') == Glyph{0, 4, 21, 14, 21, 4, 0});
}

TEST_CASE("existing glyphs keep their pixels") {
    CHECK(glyph('A') == Glyph{14, 17, 17, 31, 17, 17, 17});
    CHECK(glyph('Z') == Glyph{31, 1, 2, 4, 8, 16, 31});
    CHECK(glyph('0') == Glyph{14, 17, 19, 21, 25, 17, 14});
    CHECK(glyph('9') == Glyph{14, 17, 17, 15, 1, 1, 14});
    CHECK(glyph('/') == Glyph{1, 1, 2, 4, 8, 16, 16});
    CHECK(glyph('+') == Glyph{0, 4, 4, 31, 4, 4, 0});
    CHECK(glyph('[') == Glyph{14, 8, 8, 8, 8, 8, 14});
    CHECK(glyph(']') == Glyph{14, 2, 2, 2, 2, 2, 14});
    CHECK(glyph(':') == Glyph{0, 4, 4, 0, 4, 4, 0});
}

TEST_CASE("lowercase letters draw with the uppercase glyph") {
    for (char c = 'a'; c <= 'z'; ++c) {
        CAPTURE(c);
        CHECK(glyph(c) == glyph(static_cast<char>(c - 'a' + 'A')));
    }
}

TEST_CASE("unsupported characters are blank") {
    for (char c : {' ', '!', '?', '#', '\n', '\0', '~'}) {
        CAPTURE(static_cast<int>(c));
        CHECK(glyph(c) == Glyph{});
    }
}

TEST_CASE("glyph is usable at compile time") {
    static_assert(glyph('-')[3] == 31);
    static_assert(glyph('a') == glyph('A'));
}

TEST_CASE("zoom steps multiply by the zoom factor and clamp to the limits") {
    kinetra::Camera2D camera;
    camera.pixels_per_unit = 20;
    kinetra::zoom(camera, 1, {5, 100});
    CHECK(camera.pixels_per_unit == doctest::Approx(22));
    kinetra::zoom(camera, -2, {5, 100});
    CHECK(camera.pixels_per_unit == doctest::Approx(22 / 1.21));
    kinetra::zoom(camera, 100, {5, 100});
    CHECK(camera.pixels_per_unit == 100);
    kinetra::zoom(camera, -100, {5, 100});
    CHECK(camera.pixels_per_unit == 5);
}

TEST_CASE("zero zoom steps still apply new limits") {
    kinetra::Camera2D camera;
    camera.pixels_per_unit = 3;
    kinetra::zoom(camera, 0, {17, 100});
    CHECK(camera.pixels_per_unit == 17);
}

TEST_CASE("the rate counter publishes once per window") {
    kinetra::RateCounter frames(0.5);
    CHECK(frames.rate() == 0);
    // Binary fractions keep the window arithmetic exact.
    for (int i = 0; i < 3; ++i) {
        frames.count();
        frames.advance(0.125);
    }
    CHECK_MESSAGE(frames.rate() == 0, "no complete window yet");
    frames.count();
    frames.advance(0.125);
    CHECK(frames.rate() == doctest::Approx(8));
    frames.count(30);
    frames.advance(0.25);
    CHECK_MESSAGE(frames.rate() == doctest::Approx(8), "the rate holds until the next window closes");
    frames.advance(0.5);
    CHECK(frames.rate() == doctest::Approx(40));
}

TEST_CASE("the rate counter rejects invalid time") {
    CHECK_THROWS_AS(kinetra::RateCounter(0), std::invalid_argument);
    kinetra::RateCounter counter;
    CHECK_THROWS_AS(counter.advance(-1), std::invalid_argument);
}
