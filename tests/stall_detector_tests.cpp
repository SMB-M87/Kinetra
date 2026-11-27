// Contract tests for the stall detector: episodes, their diagnosis and when they end. Every
// case but the invalid-input one runs at two world scales, the second away from the origin.
#include <kinetra/simulation/stall_detector.h>

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>
#include <stdexcept>
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
StallObservation settings(const Frame &frame) {
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

/// A mover of half extent 0.5 at `position`, heading for `target`, with `velocity` in scene
/// units per step.
StallMover mover(const Frame &frame, Vec2 position, Vec2 target, Vec2 velocity = {},
                 StallMoverState state = StallMoverState::observed) {
    StallMover m;
    m.body.shape = Shape::box;
    m.body.motion = Motion::controlled;
    m.body.position = frame.at(position);
    m.body.half_extents = Vec2{0.5f, 0.5f} * frame.scale;
    m.body.velocity = velocity * frame.scale;
    m.target = frame.at(target);
    m.destination = m.target;
    m.state = state;
    return m;
}

/// Observes `movers` for `steps` steps, each a step more without progress for every mover,
/// and returns the events of all of them.
std::vector<StallEvent> observe(StallDetector &detector, std::vector<StallMover> &movers,
                                const StallObservation &tuning, std::uint64_t steps) {
    std::vector<StallEvent> events;
    for (std::uint64_t step = 0; step < steps; ++step) {
        for (auto &m : movers)
            ++m.no_progress;
        const auto now = movers.front().no_progress;
        for (const auto &event : detector.observe(movers, tuning, now))
            events.push_back(event);
    }
    return events;
}
} // namespace

TEST_CASE("an episode starts after the threshold and ends when progress resumes") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        StallDetector detector;
        std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0})};
        auto events = observe(detector, movers, tuning, tuning.threshold_steps - 1);
        CHECK(events.empty());
        CHECK_FALSE(detector.episode(0));
        events = observe(detector, movers, tuning, 1);
        REQUIRE(events.size() == 1);
        CHECK(events[0].kind == StallEvent::Kind::started);
        CHECK(events[0].mover == 0);
        CHECK(events[0].diagnosis.detected_at == tuning.threshold_steps);
        CHECK(events[0].diagnosis.reason == StallReason::unknown);
        CHECK_FALSE(events[0].diagnosis.blocker);
        REQUIRE(detector.episode(0));
        CHECK(observe(detector, movers, tuning, 5).empty());

        movers[0].no_progress = 0;
        events = detector.observe(movers, tuning, 100);
        REQUIRE(events.size() == 1);
        CHECK(events[0].kind == StallEvent::Kind::ended);
        CHECK(events[0].diagnosis.detected_at == tuning.threshold_steps);
        CHECK_FALSE(detector.episode(0));
    }
}

TEST_CASE("a standing mover on the leg within reach is a stationary blocker") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        StallDetector detector;
        std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}), mover(frame, {1.2f, 0.3f}, {1.2f, 0.3f},
                                                                           {}, StallMoverState::standing)};
        const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
        REQUIRE(events.size() == 1);
        CHECK(events[0].diagnosis.reason == StallReason::stationary_blocker);
        CHECK(events[0].diagnosis.blocker == 1u);
    }
}

TEST_CASE("a standing mover behind the mover, beside its leg or beyond reach blocks nothing") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        for (const Vec2 place : {Vec2{-1.2f, 0}, Vec2{1.5f, 1.2f}, Vec2{3.0f, 0}}) {
            CAPTURE(place.x);
            CAPTURE(place.y);
            StallDetector detector;
            std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}),
                                           mover(frame, place, place, {}, StallMoverState::standing)};
            const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
            REQUIRE(events.size() == 1);
            CHECK(events[0].diagnosis.reason == StallReason::unknown);
        }
    }
}

TEST_CASE("a round standing mover blocks through its bounding box") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        StallDetector detector;
        auto round = mover(frame, {1.2f, 0}, {1.2f, 0}, {}, StallMoverState::standing);
        round.body.shape = Shape::circle;
        round.body.radius = 0.4f * frame.scale;
        std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}), round};
        const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
        REQUIRE(events.size() == 1);
        CHECK(events[0].diagnosis.reason == StallReason::stationary_blocker);
    }
}

TEST_CASE("two observed movers head-on without headway oppose each other") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        StallDetector detector;
        // Sideways motion is no headway: reciprocal avoidance can keep a pair sliding in place.
        std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}, {0, 0.02f}),
                                       mover(frame, {1.1f, 0}, {-5, 0}, {0, -0.02f})};
        const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
        REQUIRE(events.size() == 2);
        CHECK(events[0].mover == 0);
        CHECK(events[0].diagnosis.reason == StallReason::opposing_mover);
        CHECK(events[0].diagnosis.blocker == 1u);
        CHECK(events[1].diagnosis.reason == StallReason::opposing_mover);
        CHECK(events[1].diagnosis.blocker == 0u);
    }
}

TEST_CASE("a busy mover, one making headway, or one on a crossing leg opposes nothing") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        SUBCASE("busy") {
            StallDetector detector;
            std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}),
                                           mover(frame, {1.1f, 0}, {-5, 0}, {}, StallMoverState::busy)};
            const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
            REQUIRE(events.size() == 1);
            CHECK(events[0].diagnosis.reason == StallReason::unknown);
        }
        SUBCASE("headway") {
            StallDetector detector;
            std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}, {0.01f, 0}),
                                           mover(frame, {1.1f, 0}, {-5, 0})};
            const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
            REQUIRE(events.size() == 2);
            CHECK(events[0].diagnosis.reason == StallReason::unknown);
        }
        SUBCASE("crossing") {
            StallDetector detector;
            std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}), mover(frame, {1.1f, 0}, {1.1f, 5})};
            const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
            REQUIRE(events.size() == 2);
            CHECK(events[0].diagnosis.reason == StallReason::unknown);
        }
    }
}

TEST_CASE("a mover backing away from a neighbor that drives at it is pushed back") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        for (const auto state : {StallMoverState::observed, StallMoverState::busy}) {
            CAPTURE(int(state));
            StallDetector detector;
            // The neighbor's route runs through the mover's place: both creep west, the mover
            // away from its target in the east.
            std::vector<StallMover> movers{mover(frame, {0, 0}, {0.3f, 0}, {-0.03f, 0}),
                                           mover(frame, {1.2f, 0.2f}, {-5, 0}, {-0.03f, 0}, state)};
            const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
            REQUIRE(events.size() >= 1);
            CHECK(events[0].mover == 0);
            CHECK(events[0].diagnosis.reason == StallReason::pushed_back);
            CHECK(events[0].diagnosis.blocker == 1u);
        }
    }
}

TEST_CASE("no push without backing, without an approaching neighbor ahead, or beyond reach") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        struct Case {
            Vec2 velocity, neighbor, neighbor_velocity;
        };
        for (const auto &[velocity, neighbor, neighbor_velocity] :
             {Case{{-0.004f, 0}, {1.2f, 0.2f}, {-0.03f, 0}}, Case{{-0.03f, 0}, {-1.2f, 0}, {0.03f, 0}},
              Case{{-0.03f, 0}, {1.2f, 0.2f}, {0.03f, 0}}, Case{{-0.03f, 0}, {2.4f, 0}, {-0.03f, 0}}}) {
            StallDetector detector;
            std::vector<StallMover> movers{
                mover(frame, {0, 0}, {0.3f, 0}, velocity),
                mover(frame, neighbor, neighbor + Vec2{-5, 0}, neighbor_velocity, StallMoverState::busy)};
            const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
            REQUIRE(events.size() == 1);
            CHECK(events[0].diagnosis.reason != StallReason::pushed_back);
        }
    }
}

TEST_CASE("a standing blocker and an opposing mover come before a push") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        StallDetector detector;
        std::vector<StallMover> movers{
            mover(frame, {0, 0}, {5, 0}, {-0.03f, 0}),
            mover(frame, {1.2f, 0.2f}, {-5, 0}, {-0.03f, 0}, StallMoverState::busy),
            mover(frame, {1.3f, -0.3f}, {1.3f, -0.3f}, {}, StallMoverState::standing)};
        const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
        REQUIRE(events.size() == 1);
        CHECK(events[0].diagnosis.reason == StallReason::stationary_blocker);
        CHECK(events[0].diagnosis.blocker == 2u);
    }
}

TEST_CASE("two reversals of at least the excursion make an episode oscillating") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        StallDetector detector;
        std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0})};
        // North, back south past the start, north again: the axis forms, then two reversals.
        const float offsets[] = {0, 0.03f, 0.03f, 0, -0.03f, -0.03f, 0, 0.03f, 0.03f, 0.03f};
        std::vector<StallEvent> events;
        for (const float y : offsets) {
            movers[0].body.position = frame.at({0, y});
            ++movers[0].no_progress;
            for (const auto &event : detector.observe(movers, tuning, movers[0].no_progress))
                events.push_back(event);
        }
        REQUIRE(events.size() == 1);
        CHECK(events[0].diagnosis.reason == StallReason::oscillating);
    }
}

TEST_CASE("a new goal, a count that does not grow or another state starts a new observation") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        for (int change = 0; change < 4; ++change) {
            CAPTURE(change);
            StallDetector detector;
            std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0})};
            REQUIRE(observe(detector, movers, tuning, tuning.threshold_steps).size() == 1);
            if (change == 0)
                movers[0].destination = frame.at({0, 5});
            else if (change == 1)
                movers[0].no_progress = 1;
            else if (change == 2)
                movers[0].state = StallMoverState::busy;
            else
                movers[0].state = StallMoverState::absent;
            const auto events = detector.observe(movers, tuning, 99);
            REQUIRE(events.size() == 1);
            CHECK(events[0].kind == StallEvent::Kind::ended);
            CHECK_FALSE(detector.episode(0));
            // The new observation needs the whole threshold again; a step in which the mover
            // is still observed is already its first.
            const std::uint64_t first = change <= 1 ? 1 : 0;
            movers[0].state = StallMoverState::observed;
            CHECK(observe(detector, movers, tuning, tuning.threshold_steps - first - 1).empty());
            CHECK(observe(detector, movers, tuning, 1).size() == 1);
        }
    }
}

TEST_CASE("an absent mover is no blocker and clear() forgets every observation") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        StallDetector detector;
        std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}),
                                       mover(frame, {1.2f, 0}, {1.2f, 0}, {}, StallMoverState::absent)};
        const auto events = observe(detector, movers, tuning, tuning.threshold_steps);
        REQUIRE(events.size() == 1);
        CHECK(events[0].diagnosis.reason == StallReason::unknown);
        detector.clear();
        CHECK_FALSE(detector.episode(0));
        CHECK(detector.observe(movers, tuning, 50).empty());
    }
}

TEST_CASE("a shorter span ends the episodes it leaves out, and a returning index starts afresh") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        const auto tuning = settings(frame);
        StallDetector detector;
        std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0}), mover(frame, {0, 3}, {5, 3})};
        REQUIRE(observe(detector, movers, tuning, tuning.threshold_steps).size() == 2);
        auto shorter = movers;
        shorter.pop_back();
        ++shorter[0].no_progress;
        const auto events = detector.observe(shorter, tuning, 50);
        REQUIRE(events.size() == 1);
        CHECK(events[0].kind == StallEvent::Kind::ended);
        CHECK(events[0].mover == 1);
        CHECK_FALSE(detector.episode(1));
        CHECK(detector.episode(0));
        // Back with the same goal and a larger count: a new observation, not the old episode.
        movers[0] = shorter[0];
        movers[1].no_progress += 5;
        ++movers[0].no_progress;
        CHECK(detector.observe(movers, tuning, 51).empty());
        CHECK_FALSE(detector.episode(1));
    }
}

TEST_CASE("the first observation anchors at the mover even when its goal is the origin") {
    // A fresh track's goal and anchor are zero: with a goal at the world origin, it must not
    // anchor there, or a mover creeping toward the goal reads as a reversal.
    const Frame frame{{0, 0}, 1};
    const auto tuning = settings(frame);
    StallDetector detector;
    std::vector<StallMover> movers{mover(frame, {10, 0}, {0, 0})};
    const float xs[] = {10, 9.97f, 9.94f, 9.91f, 9.94f, 9.97f, 9.97f, 9.97f, 9.97f, 9.97f};
    std::vector<StallEvent> events;
    for (const float x : xs) {
        movers[0].body.position = {x, 0};
        ++movers[0].no_progress;
        for (const auto &event : detector.observe(movers, tuning, movers[0].no_progress))
            events.push_back(event);
    }
    REQUIRE(events.size() == 1);
    CHECK(events[0].diagnosis.reason == StallReason::unknown);
}

TEST_CASE("invalid settings or movers throw before anything changes") {
    const Frame frame{{0, 0}, 1};
    const auto tuning = settings(frame);
    StallDetector detector;
    std::vector<StallMover> movers{mover(frame, {0, 0}, {5, 0})};
    observe(detector, movers, tuning, tuning.threshold_steps - 1);
    const auto invalid = [&](auto change) {
        auto bad = tuning;
        change(bad);
        CHECK_THROWS_AS(detector.observe(movers, bad, 0), std::invalid_argument);
    };
    invalid([](StallObservation &s) { s.threshold_steps = 0; });
    invalid([](StallObservation &s) { s.excursion = 0; });
    invalid([](StallObservation &s) { s.oscillation_reversals = 0; });
    invalid([](StallObservation &s) { s.lookahead = -1; });
    invalid([](StallObservation &s) { s.clearance = {-1, 0}; });
    invalid([](StallObservation &s) { s.stop_speed = 0; });
    invalid([](StallObservation &s) { s.opposing_cosine = 0; });
    invalid([](StallObservation &s) { s.opposing_cosine = -1; });
    invalid([](StallObservation &s) { s.pushing_reach = std::nanf(""); });
    invalid([](StallObservation &s) { s.ahead_cosine = 1; });
    auto bad_movers = movers;
    bad_movers[0].target = {std::nanf(""), 0};
    CHECK_THROWS_AS(detector.observe(bad_movers, tuning, 0), std::invalid_argument);
    bad_movers = movers;
    bad_movers[0].state = StallMoverState(7);
    CHECK_THROWS_AS(detector.observe(bad_movers, tuning, 0), std::invalid_argument);
    bad_movers = movers;
    bad_movers[0].body.half_extents = {-1, 1};
    CHECK_THROWS_AS(detector.observe(bad_movers, tuning, 0), std::invalid_argument);
    // Nothing changed: the next valid step completes the observation.
    ++movers[0].no_progress;
    const auto events = detector.observe(movers, tuning, 1);
    REQUIRE(events.size() == 1);
    CHECK(events[0].kind == StallEvent::Kind::started);
}
