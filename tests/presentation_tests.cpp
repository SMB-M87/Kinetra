// Window-free tests for the header-only presentation helpers: the bitmap glyph table
// (bitmap_text.h), camera zoom stepping (camera.h), the rate counter, frame times and frame
// limiter (timing.h) and angle interpolation (math.h).
#include <kinetra/bitmap_text.h>
#include <kinetra/camera.h>
#include <kinetra/math.h>
#include <kinetra/timing.h>

#include <doctest/doctest.h>

#include <cmath>
#include <limits>
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

TEST_CASE("invalid zoom limits are rejected") {
    kinetra::Camera2D camera;
    CHECK_THROWS_AS(kinetra::zoom(camera, 1, {50, 25}), std::invalid_argument);
    CHECK_THROWS_AS(kinetra::zoom(camera, 1, {0, 25}), std::invalid_argument);
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

TEST_CASE("frame times keep the newest frames in order") {
    kinetra::FrameTimes times;
    CHECK(times.size() == 0);
    CHECK(times.average() == 0);
    CHECK(times.worst() == 0);
    times.record(0.25);
    times.record(0.5);
    REQUIRE(times.size() == 2);
    CHECK(times[0] == 0.25);
    CHECK(times[1] == 0.5);
    CHECK(times.average() == doctest::Approx(0.375));
    CHECK(times.worst() == 0.5);
    CHECK_THROWS_AS(times[2], std::out_of_range);
}

TEST_CASE("frame times drop the oldest frame when full") {
    kinetra::FrameTimes times;
    for (std::size_t i = 0; i < kinetra::FrameTimes::capacity + 3; ++i)
        times.record(double(i));
    REQUIRE(times.size() == kinetra::FrameTimes::capacity);
    CHECK(times[0] == 3);
    CHECK(times[times.size() - 1] == double(kinetra::FrameTimes::capacity + 2));
    CHECK(times.worst() == double(kinetra::FrameTimes::capacity + 2));
}

TEST_CASE("frame times reject invalid durations") {
    kinetra::FrameTimes times;
    CHECK_THROWS_AS(times.record(-0.001), std::invalid_argument);
    CHECK_THROWS_AS(times.record(std::nan("")), std::invalid_argument);
    CHECK(times.size() == 0);
}

TEST_CASE("angle interpolation takes the shorter way around") {
    CHECK(kinetra::lerp_angle(0, 1, 0.25f) == doctest::Approx(0.25f));
    // From just below pi to just above -pi is a small step across pi, not a full turn back.
    const float halfway = kinetra::lerp_angle(3.1f, -3.1f, 0.5f);
    CHECK(std::abs(std::remainder(halfway - 3.14159265f, 6.28318531f)) < 1e-4f);
    CHECK(kinetra::lerp_angle(0.5f, 0.5f + 6.28318531f, 0.5f) == doctest::Approx(0.5f));
}

TEST_CASE("the frame limiter spaces frames one interval apart") {
    kinetra::FrameLimiter limiter(0.010);
    CHECK(limiter.interval() == 0.010);
    CHECK_MESSAGE(limiter.next(5.000) == 5.000, "the first frame starts the series and does not wait");
    CHECK(limiter.next(5.002) == doctest::Approx(5.010));
    CHECK(limiter.next(5.013) == doctest::Approx(5.020));
    CHECK(limiter.next(5.021) == doctest::Approx(5.030));
}

TEST_CASE("a late frame does not wait and the next one makes up for it") {
    kinetra::FrameLimiter limiter(0.010);
    limiter.next(1.000);
    INFO("4 ms late: no wait, and the series keeps its deadlines");
    CHECK(limiter.next(1.014) == 1.014);
    CHECK(limiter.next(1.016) == doctest::Approx(1.020));
}

TEST_CASE("a stall starts a new series instead of a burst of short frames") {
    kinetra::FrameLimiter limiter(0.010);
    limiter.next(1.000);
    CHECK_MESSAGE(limiter.next(1.250) == 1.250, "more than one interval late");
    CHECK_MESSAGE(limiter.next(1.251) == doctest::Approx(1.260), "the next frame waits a whole interval");
}

TEST_CASE("a reset frame limiter starts over") {
    kinetra::FrameLimiter limiter(0.010);
    limiter.next(1.000);
    limiter.reset();
    CHECK(limiter.next(1.001) == 1.001);
    CHECK(limiter.next(1.002) == doctest::Approx(1.011));
}

TEST_CASE("the frame limiter rejects invalid intervals and times") {
    CHECK_THROWS_AS(kinetra::FrameLimiter(0), std::invalid_argument);
    CHECK_THROWS_AS(kinetra::FrameLimiter(-1), std::invalid_argument);
    CHECK_THROWS_AS(kinetra::FrameLimiter(std::numeric_limits<double>::infinity()), std::invalid_argument);
    kinetra::FrameLimiter limiter(0.010);
    CHECK_THROWS_AS(limiter.next(std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
}
