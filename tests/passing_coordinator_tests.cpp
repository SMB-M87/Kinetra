// Contract tests for the passing coordinator: plan formation, the plan lifecycle, waits and
// the outcomes. Every lifecycle case runs at two world scales, the second away from the origin.
#include <kinetra/simulation/passing_coordinator.h>

#include <doctest/doctest.h>

#include <algorithm>
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

/// Tuning for movers of about one scene unit that drive at most 0.1 units per step.
PassingCoordination settings(const Frame &frame) {
    const float s = frame.scale;
    return {.margins = {0.1f * s, 0.2f * s, 0.4f * s},
            .prediction = {0.1f * s, 0.02f * s, 40, 4},
            .clearance = {0.02f * s, 0.02f * s},
            .arrival_distance = 0.1f * s,
            .stop_speed = 0.02f * s,
            .resume_steps = 10,
            .timeout_steps = 200,
            .retry_steps = 30};
}

Body box(Vec2 position, Vec2 half_extents) {
    Body body;
    body.shape = Shape::box;
    body.motion = Motion::controlled;
    body.position = position;
    body.half_extents = half_extents;
    return body;
}

/// A minimal host: each mover drives toward its plan target or its destination at bounded
/// speed and acceleration, and stops there. Movers without a trip stand still.
struct Host {
    explicit Host(Frame world) : frame(world), tuning(settings(world)) {}
    Frame frame;
    PassingCoordination tuning;
    std::vector<PassingMover> movers;
    PassingCoordinator coordinator;
    std::vector<PassingEvent> events;
    PassingLaneCheck lanes = {};
    /// Least gap seen between the footprints of two movers, in scene units.
    float closest = 1e9f;

    std::size_t add(Vec2 from, Vec2 to, Vec2 half = {0.5f, 0.5f}) {
        PassingMover mover;
        mover.body = box(frame.at(from), half * frame.scale);
        mover.destination = frame.at(to);
        movers.push_back(mover);
        return movers.size() - 1;
    }

    bool travelling(const PassingMover &mover) const {
        return distance(mover.body.position, mover.destination) > tuning.arrival_distance / 2;
    }

    void step() {
        for (auto &mover : movers)
            mover.heading = mover.state == PassingMoverState::standing || !travelling(mover)
                                ? Vec2{}
                                : normalized(mover.destination - mover.body.position);
        const auto step_events = coordinator.step(movers, tuning, lanes);
        events.insert(events.end(), step_events.begin(), step_events.end());
        const float speed = tuning.prediction.speed, acceleration = tuning.prediction.acceleration;
        for (std::size_t i = 0; i < movers.size(); ++i) {
            auto &body = movers[i].body;
            if (movers[i].state == PassingMoverState::standing) {
                body.velocity = {};
                continue;
            }
            const auto target = coordinator.target(i);
            const Vec2 goal = target ? target->point : movers[i].destination;
            const Vec2 offset = goal - body.position;
            // Arrive: the speed that still stops at the goal under the acceleration limit.
            const float wanted = std::min(speed, std::sqrt(2 * acceleration * length(offset)));
            const Vec2 change = normalized(offset) * wanted - body.velocity;
            body.velocity += length(change) > acceleration ? normalized(change) * acceleration : change;
            if (length(offset) < tuning.arrival_distance / 4)
                body.velocity = {};
            body.position += body.velocity;
        }
        for (std::size_t a = 0; a < movers.size(); ++a)
            for (std::size_t b = a + 1; b < movers.size(); ++b) {
                // The gap between the bounds, which is what the coordinator keeps apart.
                const auto p = bounds(movers[a].body), q = bounds(movers[b].body);
                const Vec2 gap = {std::max(p.min.x - q.max.x, q.min.x - p.max.x),
                                  std::max(p.min.y - q.max.y, q.min.y - p.max.y)};
                closest = std::min(closest, std::max(gap.x, gap.y) / frame.scale);
            }
    }

    /// Steps until `plans` plans are active; the movers start too far apart for a plan.
    void form(std::size_t plans = 1) {
        for (int i = 0; i < 400 && coordinator.size() < plans; ++i)
            step();
        REQUIRE(coordinator.size() == plans);
    }

    void run(int steps) {
        for (int i = 0; i < steps; ++i)
            step();
    }

    std::size_t count(PassingEventKind kind) const {
        return std::size_t(std::ranges::count_if(events, [&](const auto &e) { return e.kind == kind; }));
    }
    std::size_t ended(PassingOutcome outcome) const {
        return std::size_t(
            std::ranges::count_if(events, [&](const auto &e) { return e.outcome == outcome; }));
    }
};

const Frame frames[]{{{0, 0}, 1}, {{2500, -1200}, 1000}};
} // namespace

TEST_CASE("a head-on pair agrees on lanes, crosses and completes without contact") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        const auto east = host.add({0, 0}, {30, 0});
        const auto west = host.add({30, 0}, {0, 0});
        host.run(900);
        CHECK(host.count(PassingEventKind::formed) == 1);
        CHECK(host.count(PassingEventKind::crossing) == 1);
        CHECK(host.ended(PassingOutcome::completed) == 1);
        CHECK(host.coordinator.size() == 0);
        CHECK(host.closest > 0);
        const auto &formed = host.events.front();
        CHECK(formed.plan == 1);
        CHECK(formed.movers[0] == east);
        CHECK(formed.movers[1] == west);
        // The lanes lie on opposite sides of the shared line.
        CHECK((formed.lanes[0].exit.y - frame.origin.y) * (formed.lanes[1].exit.y - frame.origin.y) < 0);
        for (const auto &mover : host.movers)
            CHECK(distance(mover.body.position, mover.destination) < host.tuning.arrival_distance);
    }
}

TEST_CASE("without a plan the same pair drives into each other") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.lanes = [](std::size_t, Vec2, Vec2, Vec2) { return false; };
        host.run(400);
        CHECK(host.count(PassingEventKind::formed) == 0);
        CHECK(host.closest < 0);
    }
}

TEST_CASE("the world check decides between candidate sides") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        // Only lanes that keep the first mover north of the line are clear.
        host.lanes = [&](std::size_t mover, Vec2 from, Vec2 to, Vec2) {
            return mover != 0 || (from.y >= frame.origin.y && to.y > frame.origin.y);
        };
        host.form();
        CHECK(host.events.front().lanes[0].exit.y > frame.origin.y);
        CHECK(host.coordinator.target(0)->point == host.events.front().lanes[0].entry);
        CHECK_FALSE(host.coordinator.target(0)->waiting);
        CHECK_FALSE(host.coordinator.target(2));
    }
}

TEST_CASE("a mover standing on a lane ends the plan as obstructed and delays the next one") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.form();
        // A mover appears on the first participant's exit and stays there.
        const auto lane = host.events.front().lanes[0];
        const auto parked = host.add({0, 0}, {0, 0}, {0.3f, 0.3f});
        host.movers[parked].body.position = lane.exit;
        host.movers[parked].destination = lane.exit;
        host.movers[parked].state = PassingMoverState::standing;
        host.step();
        CHECK(host.ended(PassingOutcome::obstructed) == 1);
        CHECK(host.coordinator.size() == 0);
        // The pair may not form again before the retry delay.
        host.run(int(host.tuning.retry_steps) - 1);
        CHECK(host.count(PassingEventKind::formed) == 1);
    }
}

TEST_CASE("a participant waits for crossing traffic with priority and resumes once the lane is clear") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.form();
        // A third mover in no plan crosses the first participant's lane from the south.
        const auto lane = host.events.front().lanes[0];
        const Vec2 crossing_at = lerp(lane.entry, lane.exit, 0.4f);
        const auto crosser = host.add({0, 0}, {0, 0}, {0.3f, 0.3f});
        host.movers[crosser].body.position = crossing_at - Vec2{0, 3 * frame.scale};
        host.movers[crosser].destination = crossing_at + Vec2{0, 40 * frame.scale};
        bool waited = false;
        for (int i = 0; i < 200 && host.coordinator.size() == 1; ++i) {
            host.step();
            if (const auto target = host.coordinator.target(0); target && target->waiting)
                waited = true;
        }
        CHECK(waited);
        host.run(1200);
        CHECK(host.ended(PassingOutcome::completed) == 1);
        CHECK(host.closest > 0);
    }
}

TEST_CASE("a participant that turns busy or standing ends the plan at once") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        for (const auto state : {PassingMoverState::busy, PassingMoverState::standing}) {
            CAPTURE(int(state));
            Host host(frame);
            host.add({0, 0}, {30, 0});
            host.add({30, 0}, {0, 0});
            host.form();
            host.movers[1].state = state;
            host.step();
            CHECK(host.ended(PassingOutcome::unavailable) == 1);
            CHECK_FALSE(host.coordinator.target(0));
        }
    }
}

TEST_CASE("a plan whose participants make no headway ends after the timeout") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.form();
        // The participants stand where the plan formed, as if something held them.
        const auto movers = host.movers;
        for (std::uint64_t i = 0; i <= host.tuning.timeout_steps && host.coordinator.size() == 1; ++i) {
            const auto events = host.coordinator.step(movers, host.tuning, {});
            host.events.insert(host.events.end(), events.begin(), events.end());
        }
        CHECK(host.ended(PassingOutcome::no_progress) == 1);
    }
}

TEST_CASE("a wait that outlasts the timeout ends the plan as obstructed") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.form();
        // A mover in no plan creeps sideways through the first participant's lane ahead and
        // stays there: crossing traffic with priority that never leaves.
        const auto lane = host.events.front().lanes[0];
        const Vec2 spot = lerp(lane.entry, lane.exit, 0.3f);
        const auto creeper = host.add({0, 0}, {0, 0}, {0.3f, 0.3f});
        bool waited = false;
        for (int i = 0; i < 1200 && host.coordinator.size() == 1; ++i) {
            auto &body = host.movers[creeper].body;
            body.position = host.movers[creeper].destination = spot;
            body.velocity = Vec2{0, 0.002f} * frame.scale;
            host.step();
            if (const auto target = host.coordinator.target(0); target && target->waiting)
                waited = true;
        }
        CHECK(waited);
        CHECK(host.ended(PassingOutcome::obstructed) == 1);
        CHECK(host.ended(PassingOutcome::no_progress) == 0);
    }
}

TEST_CASE("two pairs on parallel lines form separate plans that do not share space") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.add({0, 6}, {30, 6});
        host.add({30, 6}, {0, 6});
        host.form(2);
        const auto plans = host.coordinator.plans();
        CHECK(plans[0].id == 1);
        CHECK(plans[1].id == 2);
        host.run(900);
        CHECK(host.ended(PassingOutcome::completed) == 2);
        CHECK(host.closest > 0);
    }
}

TEST_CASE("cancel and clear end plans for the caller") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.add({0, 6}, {30, 6});
        host.add({30, 6}, {0, 6});
        host.form(2);
        const auto cancelled = host.coordinator.cancel(3);
        REQUIRE(cancelled.size() == 1);
        CHECK(cancelled.front().plan == 2);
        CHECK(cancelled.front().outcome == PassingOutcome::cancelled);
        // A cancelled pair may form again in the next step.
        host.step();
        CHECK(host.coordinator.size() == 2);
        const auto cleared = host.coordinator.clear();
        CHECK(cleared.size() == 2);
        CHECK(host.coordinator.size() == 0);
    }
}

TEST_CASE("an absent mover is no obstacle and ends its plan") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.form();
        // An empty slot of a pool whose old body stands on the first participant's exit,
        // where a standing mover would end the plan as obstructed.
        const auto slot = host.add({0, 0}, {0, 0}, {0.3f, 0.3f});
        host.movers[slot].body.position = host.movers[slot].destination = host.events.front().lanes[0].exit;
        host.movers[slot].state = PassingMoverState::absent;
        host.step();
        CHECK(host.ended(PassingOutcome::obstructed) == 0);
        CHECK(host.coordinator.size() == 1);
        host.movers[0].state = PassingMoverState::absent;
        host.step();
        CHECK(host.ended(PassingOutcome::unavailable) == 1);
        for (auto &mover : host.movers)
            mover.state = PassingMoverState::absent;
        CHECK_NOTHROW(host.step());
    }
}

TEST_CASE("circles, polygons and rotated boxes take part through their bounds") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        for (int shape = 0; shape < 3; ++shape) {
            CAPTURE(shape);
            Host host(frame);
            host.add({0, 0}, {30, 0});
            host.add({30, 0}, {0, 0});
            auto &body = host.movers[1].body;
            const auto position = body.position;
            if (shape == 0) {
                body.shape = Shape::circle;
                body.radius = 0.5f * frame.scale;
            } else if (shape == 1) {
                body = regular_polygon(5, 0.6f * frame.scale);
            } else {
                body.half_extents = Vec2{0.6f, 0.3f} * frame.scale;
                body.angle = 0.6f;
            }
            body.position = position;
            body.motion = Motion::controlled;
            host.run(900);
            CHECK(host.ended(PassingOutcome::completed) == 1);
            CHECK(host.closest > 0);
        }
    }
}

TEST_CASE("slow braking next to a high speed keeps every query within the grid") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        // Braking from full speed takes 10,000 steps, so the formation range spans about
        // 2,000 scene units: far more than a grid of cells near the footprint size allows.
        host.tuning.prediction.acceleration = 0.00002f * frame.scale;
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        CHECK_NOTHROW(host.step());
        CHECK(host.coordinator.size() == 1);
    }
}

TEST_CASE("a plan waits for a participant of an older plan, and an older plan ignores a younger one") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        for (const bool older_crosses : {true, false}) {
            CAPTURE(older_crosses);
            Host host(frame);
            host.add({0, 0}, {30, 0});
            host.add({30, 0}, {0, 0});
            host.add({0, 40}, {30, 40});
            host.add({30, 40}, {0, 40});
            host.form(2);
            const auto plans = host.coordinator.plans();
            REQUIRE(plans[0].id < plans[1].id);
            // A participant of one plan strays into the lane ahead of the first participant of
            // the other plan, creeping sideways. Plans are guarded oldest first, so in this one
            // step the older plan's guard sees the younger participant still in its plan.
            const auto crossing = older_crosses ? plans[0] : plans[1];
            const auto watched = older_crosses ? plans[1] : plans[0];
            auto &body = host.movers[crossing.movers[0]].body;
            body.position = lerp(watched.lanes[0].entry, watched.lanes[0].exit, 0.3f);
            body.velocity = Vec2{0, 0.002f} * frame.scale;
            const auto events = host.coordinator.step(host.movers, host.tuning, {});
            const auto target = host.coordinator.target(watched.movers[0]);
            REQUIRE(target);
            CHECK(target->waiting == older_crosses);
            // The younger plan yields to the older one, whichever of them strayed: when its own
            // participant strayed into older traffic, it ends as obstructed.
            CHECK(std::ranges::none_of(events, [&](const PassingEvent &event) {
                return event.kind == PassingEventKind::ended && event.plan == plans[0].id;
            }));
        }
    }
}

TEST_CASE("a participant far from its lanes and a long exit offset keep every query within the grid") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.form();
        // The caller moved a participant far away without cancelling its plan.
        host.movers[0].body.position = frame.at({-600, 600});
        CHECK_NOTHROW(host.step());
        Host offset(frame);
        offset.tuning.margins.exit_offset = 550 * frame.scale;
        offset.add({0, 0}, {2000, 2000});
        offset.add({30, 30}, {-2000, -2000});
        for (int i = 0; i < 100; ++i)
            CHECK_NOTHROW(offset.step());
    }
}

TEST_CASE("clear and an absent mover drop retry delays, so a new mover on the index plans at once") {
    for (const auto &frame : frames) {
        CAPTURE(frame.scale);
        Host host(frame);
        host.add({0, 0}, {30, 0});
        host.add({30, 0}, {0, 0});
        host.form();
        // A participant turns busy: the plan fails and both indices get a retry delay.
        host.movers[1].state = PassingMoverState::busy;
        host.step();
        REQUIRE(host.ended(PassingOutcome::unavailable) == 1);
        // The caller renumbers: clear(), then a new head-on pair on the same indices, close
        // enough to conflict within the formation horizon at once.
        host.coordinator.clear();
        host.movers[0].body = box(frame.at({0, 20}), Vec2{0.5f, 0.5f} * frame.scale);
        host.movers[0].destination = frame.at({30, 20});
        host.movers[1].body = box(frame.at({8, 20}), Vec2{0.5f, 0.5f} * frame.scale);
        host.movers[1].destination = frame.at({-22, 20});
        host.movers[1].state = PassingMoverState::free;
        host.step();
        CHECK(host.coordinator.size() == 1);

        // A pool slot: the second participant's slot empties for one step, then refills.
        host.movers[1].state = PassingMoverState::busy;
        host.step();
        host.movers[1].state = PassingMoverState::absent;
        host.step();
        host.movers[1].state = PassingMoverState::free;
        host.movers[0].body = box(frame.at({0, 40}), Vec2{0.5f, 0.5f} * frame.scale);
        host.movers[0].destination = frame.at({30, 40});
        host.movers[0].body.velocity = {};
        host.movers[1].body = box(frame.at({8, 40}), Vec2{0.5f, 0.5f} * frame.scale);
        host.movers[1].destination = frame.at({-22, 40});
        // Index 0 keeps its delay from the failed plan; index 1 starts afresh, so only
        // cancelling index 0 too lets the pair plan at once.
        host.coordinator.cancel(0);
        host.step();
        CHECK(host.coordinator.size() == 1);
    }
}

TEST_CASE("invalid settings and movers throw before any plan changes") {
    Host host(frames[1]);
    host.add({0, 0}, {30, 0});
    host.add({30, 0}, {0, 0});
    host.form();
    auto bad = host.tuning;
    bad.timeout_steps = 0;
    CHECK_THROWS_AS(host.coordinator.step(host.movers, bad, {}), std::invalid_argument);
    bad = host.tuning;
    bad.prediction.speed = 0;
    CHECK_THROWS_AS(host.coordinator.step(host.movers, bad, {}), std::invalid_argument);
    auto movers = host.movers;
    movers[0].heading = {std::nanf(""), 0};
    CHECK_THROWS_AS(host.coordinator.step(movers, host.tuning, {}), std::invalid_argument);
    movers = host.movers;
    movers[0].heading = {3, 4};
    CHECK_THROWS_AS(host.coordinator.step(movers, host.tuning, {}), std::invalid_argument);
    movers = host.movers;
    movers[0].state = PassingMoverState(7);
    CHECK_THROWS_AS(host.coordinator.step(movers, host.tuning, {}), std::invalid_argument);
    CHECK(host.coordinator.size() == 1);
}
