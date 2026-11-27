// Contract tests for the right-of-way fallback and the push test it shares with the stall
// detector. Every case but the invalid-input and clock-end ones runs at two world scales, the
// second away from the origin.
#include <kinetra/simulation/right_of_way.h>

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace kinetra;

namespace {
/// A world frame: `origin` is where the scene's (0, 0) lies, `scale` world units per scene unit.
struct Frame {
    Vec2 origin;
    float scale;
    Vec2 at(Vec2 point) const { return origin + point * scale; }
};

const Frame frames[] = {{{0, 0}, 1}, {{-4000, 2500}, 250}};

/// Tuning for movers of about one scene unit that drive at most 0.1 units per step.
StallObservation observation(const Frame &frame) {
    const float s = frame.scale;
    return {.threshold_steps = 10,
            .excursion = 0.02f * s,
            .oscillation_reversals = 2,
            .lookahead = 0.5f * s,
            .clearance = {0.02f * s, 0.02f * s},
            .stop_speed = 0.005f * s,
            .opposing_cosine = -0.8f,
            .pushing_reach = 1.6f,
            .ahead_cosine = 0.2f};
}

constexpr RightOfWaySettings window{40};

StallMover mover(const Frame &frame, Vec2 position, Vec2 target, Vec2 velocity, std::uint64_t no_progress,
                 StallMoverState state = StallMoverState::observed) {
    StallMover m;
    m.body.shape = Shape::box;
    m.body.motion = Motion::controlled;
    m.body.position = frame.at(position);
    m.body.half_extents = Vec2{0.5f, 0.5f} * frame.scale;
    m.body.velocity = velocity * frame.scale;
    m.target = frame.at(target);
    m.destination = m.target;
    m.no_progress = no_progress;
    m.state = state;
    return m;
}

/// Mover 0 backs away west from its target, 0.3 east, while mover 1 drives west at it.
std::vector<StallMover> pushed_pair(const Frame &frame, std::uint64_t no_progress) {
    return {mover(frame, {0, 0}, {0.3f, 0}, {-0.03f, 0}, no_progress),
            mover(frame, {1.2f, 0.2f}, {-5, 0}, {-0.03f, 0}, 0)};
}
} // namespace

TEST_CASE("a stalled mover pushed back gets the right of way for the window") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        RightOfWay fallback;
        auto movers = pushed_pair(frame, tuning.threshold_steps - 1);
        CHECK(fallback.step(movers, tuning, window, 100).empty());
        CHECK_FALSE(fallback.granted(0));
        movers[0].no_progress = tuning.threshold_steps;
        auto events = fallback.step(movers, tuning, window, 101);
        REQUIRE(events.size() == 1);
        CHECK(events[0].kind == RightOfWayEvent::Kind::granted);
        CHECK(events[0].mover == 0);
        CHECK(events[0].pusher == 1u);
        CHECK(fallback.granted(0));
        CHECK_FALSE(fallback.granted(1));

        // It keeps the right of way through the window, whatever it does, then loses it.
        movers[0].body.velocity = {};
        for (std::uint64_t now = 102; now < 101 + window.window_steps; ++now)
            CHECK(fallback.step(movers, tuning, window, now).empty());
        events = fallback.step(movers, tuning, window, 101 + window.window_steps);
        REQUIRE(events.size() == 1);
        CHECK(events[0].kind == RightOfWayEvent::Kind::ended);
        CHECK_FALSE(fallback.granted(0));
    }
}

TEST_CASE("a mover still pushed back when its window passes gets a new one at once") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        RightOfWay fallback;
        const auto movers = pushed_pair(frame, tuning.threshold_steps);
        REQUIRE(fallback.step(movers, tuning, window, 0).size() == 1);
        const auto events = fallback.step(movers, tuning, window, window.window_steps);
        REQUIRE(events.size() == 2);
        CHECK(events[0].kind == RightOfWayEvent::Kind::ended);
        CHECK(events[1].kind == RightOfWayEvent::Kind::granted);
        CHECK(fallback.granted(0));
    }
}

TEST_CASE("no right of way below the threshold, without a push, or for a mover not observed") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        SUBCASE("progress") {
            RightOfWay fallback;
            CHECK(fallback.step(pushed_pair(frame, tuning.threshold_steps - 1), tuning, window, 0).empty());
        }
        SUBCASE("no push") {
            auto movers = pushed_pair(frame, tuning.threshold_steps);
            movers[1].body.velocity = {};
            RightOfWay fallback;
            CHECK(fallback.step(movers, tuning, window, 0).empty());
        }
        SUBCASE("standing pusher") {
            auto movers = pushed_pair(frame, tuning.threshold_steps);
            movers[1].state = StallMoverState::standing;
            RightOfWay fallback;
            CHECK(fallback.step(movers, tuning, window, 0).empty());
        }
        for (const auto state : {StallMoverState::busy, StallMoverState::standing, StallMoverState::absent}) {
            CAPTURE(int(state));
            auto movers = pushed_pair(frame, tuning.threshold_steps);
            movers[0].state = state;
            RightOfWay fallback;
            CHECK(fallback.step(movers, tuning, window, 0).empty());
        }
    }
}

TEST_CASE("a mover that is no longer observed or left the span loses the right of way at once") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        for (const auto state : {StallMoverState::busy, StallMoverState::standing, StallMoverState::absent}) {
            CAPTURE(int(state));
            RightOfWay fallback;
            auto movers = pushed_pair(frame, tuning.threshold_steps);
            REQUIRE(fallback.step(movers, tuning, window, 0).size() == 1);
            movers[0].state = state;
            const auto events = fallback.step(movers, tuning, window, 1);
            REQUIRE(events.size() == 1);
            CHECK(events[0].kind == RightOfWayEvent::Kind::ended);
            CHECK_FALSE(fallback.granted(0));
        }
        RightOfWay fallback;
        auto movers = pushed_pair(frame, tuning.threshold_steps);
        std::swap(movers[0], movers[1]);
        REQUIRE(fallback.step(movers, tuning, window, 0).size() == 1);
        REQUIRE(fallback.granted(1));
        movers.pop_back();
        const auto events = fallback.step(movers, tuning, window, 1);
        REQUIRE(events.size() == 1);
        CHECK(events[0].kind == RightOfWayEvent::Kind::ended);
        CHECK(events[0].mover == 1);
        CHECK_FALSE(fallback.granted(1));
        fallback.clear();
        CHECK_FALSE(fallback.granted(0));
    }
}

TEST_CASE("the push test is the stall detector's pushed_back reason") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        auto movers = pushed_pair(frame, 0);
        CHECK(pushing_neighbor(movers, 0, tuning) == 1u);
        CHECK_FALSE(pushing_neighbor(movers, 1, tuning));
        StallDetector detector;
        std::optional<StallDiagnosis> diagnosis;
        for (std::uint64_t step = 1; step <= tuning.threshold_steps; ++step) {
            movers[0].no_progress = step;
            for (const auto &event : detector.observe(movers, tuning, step))
                if (event.mover == 0)
                    diagnosis = event.diagnosis;
        }
        REQUIRE(diagnosis);
        CHECK(diagnosis->reason == StallReason::pushed_back);
        CHECK(diagnosis->blocker == 1u);
    }
}

TEST_CASE("ends stay in mover order when a shorter span leaves holders out") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        RightOfWay fallback;
        // Two pushed pairs, far apart: movers 0 and 2 get the right of way.
        auto movers = pushed_pair(frame, tuning.threshold_steps);
        auto second = pushed_pair(frame, tuning.threshold_steps);
        for (auto &m : second) {
            m.body.position += Vec2{0, 10} * frame.scale;
            m.target += Vec2{0, 10} * frame.scale;
            m.destination += Vec2{0, 10} * frame.scale;
        }
        movers.insert(movers.end(), second.begin(), second.end());
        REQUIRE(fallback.step(movers, tuning, window, 0).size() == 2);
        movers.resize(2);
        movers[0].state = StallMoverState::busy;
        const auto events = fallback.step(movers, tuning, window, 1);
        REQUIRE(events.size() == 2);
        CHECK(events[0].mover == 0);
        CHECK(events[1].mover == 2);
    }
}

TEST_CASE("a pusher that holds the right of way against the mover gets none in return") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        RightOfWay fallback;
        auto movers = pushed_pair(frame, tuning.threshold_steps);
        REQUIRE(fallback.step(movers, tuning, window, 0).size() == 1);
        REQUIRE(fallback.granted(0));
        // Holding the right of way, mover 0 drives east at its goal; mover 1, bound west, now
        // backs off east before it: pushed back by mover 0 in turn, and stalled too.
        movers[0].body.velocity = Vec2{0.03f, 0} * frame.scale;
        movers[1].body.velocity = Vec2{0.03f, 0} * frame.scale;
        movers[1].no_progress = tuning.threshold_steps;
        REQUIRE(pushing_neighbor(movers, 1, tuning) == 0u);
        CHECK(fallback.step(movers, tuning, window, 1).empty());
        CHECK_FALSE(fallback.granted(1));
    }
}

TEST_CASE("a new goal ends the right of way at once") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        RightOfWay fallback;
        auto movers = pushed_pair(frame, tuning.threshold_steps);
        REQUIRE(fallback.step(movers, tuning, window, 0).size() == 1);
        // The mover stays observed and pushed; only its trip changes.
        movers[0].destination = frame.at({0, 5});
        const auto events = fallback.step(movers, tuning, window, 1);
        REQUIRE(events.size() == 2);
        CHECK(events[0].kind == RightOfWayEvent::Kind::ended);
        // Still stalled and pushed back on the new trip, it gets a new right of way for it.
        CHECK(events[1].kind == RightOfWayEvent::Kind::granted);
        movers[0].no_progress = 0;
        movers[0].destination = frame.at({0, -5});
        REQUIRE(fallback.step(movers, tuning, window, 2).size() == 1);
        CHECK_FALSE(fallback.granted(0));
    }
}

TEST_CASE("a window past the end of the clock lasts to its end") {
    const Frame frame{{0, 0}, 1};
    const auto tuning = observation(frame);
    RightOfWay fallback;
    const auto movers = pushed_pair(frame, tuning.threshold_steps);
    const RightOfWaySettings endless{std::numeric_limits<std::uint64_t>::max()};
    REQUIRE(fallback.step(movers, tuning, endless, 5).size() == 1);
    CHECK(fallback.step(movers, tuning, endless, 6).empty());
    CHECK(fallback.granted(0));
}

TEST_CASE("the push test for every mover agrees with the one for each") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = observation(frame);
        auto movers = pushed_pair(frame, 0);
        movers.push_back(mover(frame, {0, 3}, {5, 3}, {}, 0));
        const auto all = pushing_neighbors(movers, tuning);
        REQUIRE(all.size() == movers.size());
        for (std::size_t i = 0; i < movers.size(); ++i)
            CHECK(all[i] == pushing_neighbor(movers, i, tuning));
        CHECK(all[0] == 1u);
    }
}

TEST_CASE("invalid input throws before anything changes") {
    const Frame frame{{0, 0}, 1};
    const auto tuning = observation(frame);
    RightOfWay fallback;
    const auto movers = pushed_pair(frame, tuning.threshold_steps);
    // A right of way exists before the invalid calls, so that one changing anything shows.
    REQUIRE(fallback.step(movers, tuning, window, 0).size() == 1);
    CHECK_THROWS_AS(fallback.step(movers, tuning, {0}, 0), std::invalid_argument);
    auto bad = tuning;
    bad.stop_speed = 0;
    CHECK_THROWS_AS(fallback.step(movers, bad, window, 0), std::invalid_argument);
    auto bad_movers = movers;
    bad_movers[1].destination = {std::nanf(""), 0};
    CHECK_THROWS_AS(fallback.step(bad_movers, tuning, window, 0), std::invalid_argument);
    CHECK_THROWS_AS(pushing_neighbor(movers, 2, tuning), std::out_of_range);
    // Nothing changed: the right of way holds, and ends at the end of its window.
    CHECK(fallback.granted(0));
    CHECK(fallback.step(movers, tuning, window, 1).empty());
    const auto events = fallback.step(movers, tuning, window, window.window_steps);
    REQUIRE_FALSE(events.empty());
    CHECK(events[0].kind == RightOfWayEvent::Kind::ended);
}
