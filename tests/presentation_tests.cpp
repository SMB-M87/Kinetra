// Window-free tests for the header-only presentation helpers: the bitmap glyph table
// (bitmap_text.h), the HUD lines and error wrapping (hud_text.h), camera zoom stepping (camera.h), the rate
// counter, frame times and frame limiter (timing.h), angle interpolation (math.h) and debug-menu rows
// (debug_menu.h).
#include <kinetra/runtime/bitmap_text.h>
#include <kinetra/runtime/hud_text.h>
#include <kinetra/simulation/math.h>
#include <kinetra/view/camera.h>
#include <kinetra/view/debug_menu.h>
#include <kinetra/view/inspector.h>
#include <kinetra/view/timing.h>

#include <doctest/doctest.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using kinetra::Glyph;
using kinetra::glyph;

TEST_CASE("headless inspector snapshots own ordered rows with formatted units") {
    const auto build = [] {
        std::string title = "MOVER";
        std::string speed = kinetra::inspector_number(1.23456, 4) + " MM/TICK";
        kinetra::InspectorSnapshot snapshot{
            title, {{"MOTION", {{"SPEED", speed}, {"GOAL", kinetra::inspector_vector({-2.5f, 3}, " MM")}}}}};
        title.assign("OTHER");
        speed.assign("STALE");
        return snapshot;
    };
    const auto snapshot = build();
    CHECK(snapshot.title == "MOVER");
    REQUIRE(snapshot.groups.size() == 1);
    CHECK(snapshot.groups.front().heading == "MOTION");
    const auto &rows = snapshot.groups.front().rows;
    REQUIRE(rows.size() == 2);
    CHECK(rows[0].label == "SPEED");
    CHECK(rows[0].value == "1.235 MM/TICK");
    CHECK(rows[1].label == "GOAL");
    CHECK(rows[1].value == "-2.5, 3 MM");
}

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

TEST_CASE("a parser diagnostic's punctuation has glyphs") {
    // nlohmann's message for a blueprint cut off after "{ \"name\": ".
    const std::string_view diagnostic = "[json.exception.parse_error.101] parse error at line 1, column 11: "
                                        "syntax error while parsing value - "
                                        "unexpected end of input; expected '[', '{', or a literal";
    for (char c : diagnostic) {
        CAPTURE(c);
        CHECK_MESSAGE((c == ' ' || glyph(c) != Glyph{}), "every printed character of the diagnostic shows");
    }
    for (char c : {'{', '}', '(', ')', '\'', '"', ';'}) {
        CAPTURE(c);
        CHECK(glyph(c) != Glyph{});
    }
    CHECK_MESSAGE(glyph('{') != glyph('['), "braces differ from brackets");
    CHECK_MESSAGE(glyph(';') != glyph(':'), "a semicolon differs from a colon");
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

TEST_CASE("text width counts six pixels per character less the last gap") {
    CHECK(kinetra::text_width("") == 0);
    CHECK(kinetra::text_width("A") == 5);
    CHECK(kinetra::text_width("DEBUG") == 29);
}

TEST_CASE("HUD lines count up from the bottom of the viewport") {
    namespace hud = kinetra::hud;
    const kinetra::Vec2 viewport{1280, 720};
    CHECK(hud::line_top(viewport, 1) == 700);
    CHECK(hud::line_top(viewport, 3) == 660);
    CHECK_MESSAGE(hud::line_top(viewport, 1) + 7 * hud::text_scale <= viewport.y,
                  "line 1 is inside the window");
    CHECK(hud::lines_below(viewport, 660) == 3);
    CHECK(hud::lines_below(viewport, 661) == 2);
    CHECK(hud::lines_below(viewport, 720) == 0);
    CHECK(hud::lines_below(viewport, 800) == 0);
}

TEST_CASE("HUD columns fit the width between the margins and are at least one") {
    namespace hud = kinetra::hud;
    // Nine screen pixels per character at scale 1.5, ten pixels of margin on each side.
    CHECK(hud::columns(1280) == 140);
    CHECK(hud::columns(29) == 1);
    CHECK(hud::columns(0) == 1);
    CHECK(hud::columns(-5) == 1);
}

TEST_CASE("error text wraps at spaces and cuts words longer than a line") {
    using kinetra::hud::wrap;
    using Lines = std::vector<std::string_view>;
    CHECK(wrap("", 10).empty());
    CHECK((wrap("SHORT", 10) == Lines{"SHORT"}));
    CHECK((wrap("GOAL REJECTED: MOVER BUSY", 14) == Lines{"GOAL REJECTED:", "MOVER BUSY"}));
    CHECK((wrap("ONE TWO THREE", 7) == Lines{"ONE TWO", "THREE"}));
    CHECK((wrap("/A/VERY/LONG/PATH.JSON", 8) == Lines{"/A/VERY/", "LONG/PAT", "H.JSON"}));
    CHECK_MESSAGE((wrap(" LEADING", 4) == Lines{" LEA", "DING"}), "a space at the start gives no empty line");
    CHECK_MESSAGE((wrap("ONE  TWO", 3) == Lines{"ONE", "TWO"}), "a run of spaces at a break goes");
    CHECK_MESSAGE((wrap("ONE   ", 3) == Lines{"ONE"}), "trailing spaces give no empty line");
    CHECK_MESSAGE((wrap("ONE  TWO", 4) == Lines{"ONE", "TWO"}), "a line keeps no space before its break");
    CHECK_MESSAGE((wrap("  LEADING", 4) == Lines{"  LE", "ADIN", "G"}),
                  "a break before the first word counts as no break");
    CHECK_MESSAGE(wrap("   ", 4).empty(), "only spaces give no line");
    CHECK_THROWS_AS(wrap("TEXT", 0), std::invalid_argument);
}

TEST_CASE("an error longer than its lines ends with the cut mark") {
    using kinetra::hud::error_lines;
    using Lines = std::vector<std::string>;
    CHECK((error_lines("ONE TWO THREE FOUR", 9, 4) == Lines{"ONE TWO", "THREE", "FOUR"}));
    CHECK((error_lines("ONE TWO THREE FOUR", 9, 2) == Lines{"ONE TWO", "THREE..."}));
    CHECK((error_lines("ABCDEFGHIJKL", 6, 1) == Lines{"ABC..."}));
    CHECK(error_lines("ONE TWO", 9, 0).empty());
    CHECK(error_lines("", 9, 3).empty());
    CHECK_MESSAGE((error_lines("ABCDEFGH", 2, 1) == Lines{".."}),
                  "a line narrower than the mark keeps what fits");
    CHECK((error_lines("ABCDEFGH", 3, 1) == Lines{"..."}));
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

TEST_CASE("debug folds keep one section open and a click on it closes it") {
    kinetra::DebugFolds folds("SIMULATION");
    CHECK(folds.section_open("SIMULATION"));
    auto simulation = folds.section("SIMULATION");
    auto motion = folds.section("MOTION");
    CHECK(simulation.kind == kinetra::DebugEntryKind::group);
    CHECK(simulation.depth == 0);
    CHECK(simulation.active());
    CHECK_FALSE(motion.active());
    motion.action();
    CHECK(folds.section_open("MOTION"));
    CHECK_FALSE(folds.section_open("SIMULATION"));
    CHECK_FALSE(simulation.active());
    motion.action();
    CHECK_FALSE(folds.section_open("MOTION"));
    CHECK_MESSAGE(!kinetra::DebugFolds().section_open("SIMULATION"), "no section is open by default");
}

TEST_CASE("a debug list shows its value and closes with its section or after a choice") {
    kinetra::DebugFolds folds("MOTION");
    std::string profile = "IMPROVED";
    auto list = folds.list("PROFILE", [&] { return profile; });
    CHECK(list.kind == kinetra::DebugEntryKind::group);
    CHECK(list.depth == 1);
    CHECK(list.value() == "IMPROVED");
    CHECK_FALSE(folds.list_open("PROFILE"));
    list.action();
    CHECK(folds.list_open("PROFILE"));
    CHECK(list.active());
    folds.close_list();
    CHECK_FALSE(folds.list_open("PROFILE"));

    list.action();
    folds.list("PLANNER", [] { return std::string("A*"); }).action();
    CHECK_MESSAGE(!folds.list_open("PROFILE"), "one list is open at a time");
    CHECK(folds.list_open("PLANNER"));
    folds.section("TRAFFIC").action();
    CHECK_MESSAGE(!folds.list_open("PLANNER"), "a list belongs to the section it was opened in");
}

TEST_CASE("a debug check row flips its borrowed flag") {
    bool flag = false;
    auto row = kinetra::debug_check("VECTORS", flag, 2);
    CHECK(row.kind == kinetra::DebugEntryKind::check);
    CHECK(row.depth == 2);
    CHECK_FALSE(row.active());
    row.action();
    CHECK(flag);
    CHECK(row.active());
}

TEST_CASE("a debug number row steps by an amount or a factor and stops at its limits") {
    float clearance = 0.2f;
    auto added = kinetra::debug_number("CLEARANCE", clearance, {0, 0.5f, 0.1f});
    CHECK_FALSE(added.action);
    added.increase();
    CHECK(clearance == doctest::Approx(0.3f));
    for (int i = 0; i < 10; ++i)
        added.increase();
    CHECK(clearance == 0.5f);
    for (int i = 0; i < 10; ++i)
        added.decrease();
    CHECK(clearance == 0);

    float speed = 4;
    auto scaled = kinetra::debug_number("MAX SPEED", speed, {1, 8, 1.25f, true});
    scaled.increase();
    CHECK(speed == 5);
    scaled.decrease();
    CHECK_MESSAGE(speed == 4, "a step up and a step down return to the same value");
    scaled.decrease();
    CHECK(speed == doctest::Approx(3.2f));
    CHECK(scaled.value() == "3.2");
    for (int i = 0; i < 20; ++i)
        scaled.increase();
    CHECK(speed == 8);
}

TEST_CASE("a debug count row steps by one within its limits") {
    int hits = 2;
    auto row = kinetra::debug_count("HITS", hits, 1, 3);
    CHECK(row.value() == "2");
    row.increase();
    row.increase();
    CHECK(hits == 3);
    for (int i = 0; i < 5; ++i)
        row.decrease();
    CHECK(hits == 1);
}

TEST_CASE("debug number text keeps three significant digits") {
    CHECK(kinetra::debug_number_text(2) == "2");
    CHECK(kinetra::debug_number_text(0.6f) == "0.6");
    CHECK(kinetra::debug_number_text(0.0512f) == "0.0512");
    CHECK(kinetra::debug_number_text(1.953125f) == "1.95");
    CHECK(kinetra::debug_number_text(123.456f) == "123");
    CHECK_MESSAGE(kinetra::debug_number_text(12345.6f) == "12346", "no exponent from 1000 on");
}

TEST_CASE("invalid debug number steps are rejected") {
    float value = 1;
    int count = 1;
    CHECK_THROWS_AS(kinetra::debug_number("X", value, {2, 1, 0.1f}), std::invalid_argument);
    CHECK_THROWS_AS(kinetra::debug_number("X", value, {0, 1, 0}), std::invalid_argument);
    CHECK_THROWS_AS(kinetra::debug_number("X", value, {0, 1, 1, true}), std::invalid_argument);
    CHECK_THROWS_AS(kinetra::debug_number("X", value, {0, std::numeric_limits<float>::infinity(), 1}),
                    std::invalid_argument);
    CHECK_THROWS_AS(kinetra::debug_count("X", count, 3, 1), std::invalid_argument);
}
