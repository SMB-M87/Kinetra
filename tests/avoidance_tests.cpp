// Contract tests for kinetra::avoid_collisions (include/kinetra/avoidance.h): motion limits,
// infeasibility, deterministic IDs, time units, rectangular constraints, and seeded crowd
// scenarios that must arrive without physical overlap.
#include <kinetra/avoidance.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

using namespace kinetra;

namespace {
/// Agent shape used by a case: a circle, or an axis-aligned rectangle inside its circle.
enum class Footprint { circle, rectangle };

/// Resting circular agent at the origin.
AvoidanceAgent resting_agent() {
    return {0, {}, {}, 0.4f};
}

/// Limits under which one 0.1 s step from rest reaches exactly 0.1 units/s.
AvoidanceSettings unit_limits() {
    AvoidanceSettings settings;
    settings.max_speed = 2;
    settings.max_acceleration = 1;
    return settings;
}

/// Circular agent at the origin moving right at 2 units/s.
AvoidanceAgent moving_agent() {
    return {0, {}, {2, 0}, 0.4f};
}

/// Two neighbors whose courses cross the path of moving_agent().
std::vector<AvoidanceAgent> crossing_neighbors() {
    return {{1, {2, 0.1f}, {-1, 0}, 0.4f}, {2, {1, -1}, {0, 1}, 0.4f}};
}

/// Avoidance of one encounter, once in seconds and once rescaled to 60 Hz ticks.
struct TimeUnitResults {
    AvoidanceResult seconds;
    AvoidanceResult ticks;
};

/// Avoids a head-on encounter in seconds, then again with every time-dependent value
/// rescaled to 60 Hz ticks, so a consistent conversion must give the same motion.
TimeUnitResults same_encounter_in_seconds_and_ticks(Footprint footprint) {
    AvoidanceAgent a{0, {}, {2, 0}, 0.4f}, b{1, {3, 0.2f}, {-1, 0}, 0.4f};
    AvoidanceSettings settings;
    if (footprint == Footprint::rectangle) {
        a.half_extents = {0.3f, 0.1f};
        b.half_extents = {0.2f, 0.3f};
    }
    const auto seconds = avoid_collisions(a, {2, 0}, std::span<const AvoidanceAgent>(&b, 1), 0.1f, settings);
    a.velocity = a.velocity / 60;
    b.velocity = b.velocity / 60;
    settings.max_speed /= 60;
    settings.max_acceleration /= 3600;
    settings.horizon *= 60;
    const auto ticks =
        avoid_collisions(a, {2.f / 60, 0}, std::span<const AvoidanceAgent>(&b, 1), 6, settings);
    return {seconds, ticks};
}

/// Whether moving from the origin by `travel` enters the box `center` +- `extent`.
bool enters_rectangle(Vec2 travel, Vec2 center, Vec2 extent) {
    float enter = 0, leave = 1;
    for (int axis = 0; axis < 2; ++axis) {
        const float v = axis ? travel.y : travel.x;
        const float low = (axis ? center.y - extent.y : center.x - extent.x);
        const float high = (axis ? center.y + extent.y : center.x + extent.x);
        if (v == 0) {
            if (low > 0 || high < 0)
                return false;
        } else {
            enter = std::max(enter, std::min(low / v, high / v));
            leave = std::min(leave, std::max(low / v, high / v));
        }
    }
    return enter <= leave;
}

/// Settings that make rectangle constraints, not speed or acceleration, the binding limit.
AvoidanceSettings rectangle_settings() {
    AvoidanceSettings settings;
    settings.max_speed = 3;
    settings.max_acceleration = 100;
    settings.horizon = 2;
    settings.clearance = 0.02f;
    settings.passing_bias = 0;
    return settings;
}

/// Two identical resting rectangles at the origin; only `a` cooperates.
struct OverlappingRectangles {
    AvoidanceAgent a{0, {}, {}, 1, true, {0.6f, 0.15f}};
    AvoidanceAgent b{1, {}, {}, 1, false, {0.6f, 0.15f}};
    AvoidanceSettings settings;
};

/// The overlapping pair with an acceleration limit of 0.1.
OverlappingRectangles overlapping_rectangles() {
    OverlappingRectangles pair;
    pair.settings = rectangle_settings();
    pair.settings.max_acceleration = 0.1f;
    return pair;
}

/// Agents of a crowd scenario with the goal of each agent at the same index.
struct Crowd {
    std::vector<AvoidanceAgent> agents;
    std::vector<Vec2> goals;
    Footprint footprint{Footprint::circle};
};

/// `count` agents evenly spaced on a ring of radius 6, each heading for the opposite point.
Crowd ring(int count, Footprint footprint) {
    Crowd crowd;
    crowd.footprint = footprint;
    for (int i = 0; i < count; ++i) {
        const Vec2 position =
            rotated({-6, 0}, 6.283185307f * static_cast<float>(i) / static_cast<float>(count));
        crowd.agents.push_back({std::uint64_t(i), position, {}, 0.4f});
        crowd.goals.push_back(-position);
    }
    if (footprint == Footprint::rectangle)
        for (auto &agent : crowd.agents) {
            agent.half_extents = {0.35f, 0.15f};
            agent.radius = length(agent.half_extents);
        }
    return crowd;
}

/// One agent crossing the ring through a large parked, noncooperative body at its center.
Crowd parked_crossing() {
    auto crowd = ring(2, Footprint::circle);
    crowd.agents.resize(1);
    crowd.goals.resize(1);
    crowd.agents.push_back({1, {}, {}, 1, false});
    crowd.goals.push_back({});
    return crowd;
}

/// Limits shared by the crowd scenarios.
AvoidanceSettings crowd_settings() {
    AvoidanceSettings settings;
    settings.max_speed = 2;
    settings.max_acceleration = 8;
    settings.horizon = 2;
    settings.clearance = 0.15f;
    return settings;
}

/// Physical gap between two agents; negative when they overlap.
float physical_gap(const AvoidanceAgent &a, const AvoidanceAgent &b, Footprint footprint) {
    if (footprint == Footprint::rectangle) {
        const auto delta = a.position - b.position;
        const auto extent = a.half_extents + b.half_extents;
        return std::max(std::abs(delta.x) - extent.x, std::abs(delta.y) - extent.y);
    }
    return length(a.position - b.position) - a.radius - b.radius;
}

/// Runs the crowd for 1200 ticks at 60 Hz, every cooperative agent avoiding one shared
/// snapshot per tick. Requires every result to respect the motion limits and returns the
/// smallest gap seen between any two agents.
float simulate(Crowd &crowd, const AvoidanceSettings &settings) {
    auto &agents = crowd.agents;
    float minimum_gap = 100;
    for (int tick = 0; tick < 1200; ++tick) {
        const auto snapshot = agents;
        for (std::size_t i = 0; i < agents.size(); ++i) {
            if (!agents[i].cooperative)
                continue;
            std::vector<AvoidanceAgent> neighbors;
            for (std::size_t j = 0; j < snapshot.size(); ++j)
                if (i != j)
                    neighbors.push_back(snapshot[j]);
            const auto delta = crowd.goals[i] - agents[i].position;
            const auto preferred = normalized(delta) * std::min(settings.max_speed, length(delta) * 2);
            const auto result = avoid_collisions(snapshot[i], preferred, neighbors, 1.f / 60, settings);
            REQUIRE_MESSAGE(length(result.acceleration) <= settings.max_acceleration + 0.001f,
                            "Scenario violated motion limits");
            REQUIRE_MESSAGE(length(result.velocity) <= settings.max_speed + 0.001f,
                            "Scenario violated motion limits");
            agents[i].velocity = result.velocity;
            agents[i].position += result.velocity / 60;
        }
        for (std::size_t i = 0; i < agents.size(); ++i)
            for (std::size_t j = i + 1; j < agents.size(); ++j)
                minimum_gap = std::min(minimum_gap, physical_gap(agents[i], agents[j], crowd.footprint));
    }
    return minimum_gap;
}

/// Runs `crowd` and checks that no agents overlap and every cooperative agent stops at its goal.
void check_crowd(Crowd crowd) {
    const float minimum_gap = simulate(crowd, crowd_settings());
    CHECK_MESSAGE(minimum_gap >= -0.001f, "Predictive avoidance allowed physical overlap");
    for (std::size_t i = 0; i < crowd.agents.size(); ++i) {
        if (!crowd.agents[i].cooperative)
            continue;
        CAPTURE(i);
        CHECK(length(crowd.agents[i].position - crowd.goals[i]) < 0.3f);
        CHECK(length(crowd.agents[i].velocity) < 0.05f);
    }
}
} // namespace

TEST_CASE("free acceleration is bounded by the acceleration limit") {
    const auto result = avoid_collisions(resting_agent(), {2, 0}, {}, 0.1f, unit_limits());
    CHECK(result.feasible);
    CHECK(length(result.acceleration) <= 1.00001f);
    CHECK(std::abs(result.velocity.x - 0.1f) < 0.00001f);
}

TEST_CASE("overspeed decelerates without instantaneous clamping") {
    auto agent = resting_agent();
    agent.velocity = {10, 0};
    const auto result = avoid_collisions(agent, {2, 0}, {}, 0.1f, unit_limits());
    CHECK(result.feasible);
    CHECK(std::abs(result.velocity.x - 9.9f) < 0.00001f);
}

TEST_CASE("infeasible avoidance stays bounded and reports failure") {
    const std::vector<AvoidanceAgent> trapped{{1, {-0.1f, 0}, {}, 0.4f, false},
                                              {2, {0.1f, 0}, {}, 0.4f, false}};
    const auto result = avoid_collisions(resting_agent(), {2, 0}, trapped, 0.01f, unit_limits());
    CHECK_FALSE(result.feasible);
    CHECK(result.violation > 0);
    CHECK(finite(result.velocity));
    CHECK(length(result.acceleration) <= 1.00001f);
}

TEST_CASE("coincident agents recover in a stable ID-based direction") {
    const AvoidanceAgent coincident{1, {}, {}, 0.4f, false};
    const auto result = avoid_collisions(resting_agent(), {}, {&coincident, 1}, 0.01f, unit_limits());
    CHECK_FALSE(result.feasible);
    CHECK(result.velocity.x < 0);
}

TEST_CASE("a zero speed limit allows no motion") {
    auto settings = unit_limits();
    settings.max_speed = 0;
    const auto result = avoid_collisions(resting_agent(), {0.0001f, 0}, {}, 0.1f, settings);
    CHECK(length(result.velocity) == 0);
}

TEST_CASE("a zero acceleration limit keeps the velocity unchanged") {
    auto settings = unit_limits();
    settings.max_acceleration = 0;
    const auto result = avoid_collisions(resting_agent(), {2, 0}, {}, 0.01f, settings);
    CHECK(length(result.velocity) == 0);
    CHECK(length(result.acceleration) == 0);
}

TEST_CASE("neighbor order does not change avoidance") {
    auto neighbors = crossing_neighbors();
    const auto forward = avoid_collisions(moving_agent(), {2, 0}, neighbors, 0.1f);
    std::reverse(neighbors.begin(), neighbors.end());
    const auto reversed = avoid_collisions(moving_agent(), {2, 0}, neighbors, 0.1f);
    CHECK(length(forward.velocity - reversed.velocity) < 0.00001f);
    CHECK(forward.feasible == reversed.feasible);
}

TEST_CASE("duplicate neighbor IDs are rejected") {
    auto neighbors = crossing_neighbors();
    std::reverse(neighbors.begin(), neighbors.end());
    neighbors[0].id = neighbors[1].id;
    CHECK_THROWS_AS(avoid_collisions(moving_agent(), {}, neighbors, 0.1f), std::invalid_argument);
}

TEST_CASE("a zero timestep is rejected") {
    CHECK_THROWS_AS(avoid_collisions(moving_agent(), {}, {}, 0), std::invalid_argument);
}

TEST_CASE("an agent passes a noncooperative moving neighbor and reaches its goal") {
    AvoidanceAgent a{0, {-6, 0}, {}, 0.4f}, b{1, {0, -6}, {0, 2}, 0.4f, false};
    AvoidanceSettings settings;
    settings.max_speed = 2;
    settings.max_acceleration = 8;
    settings.horizon = 2;
    float gap = 100;
    for (int tick = 0; tick < 1200; ++tick) {
        const Vec2 target{6, 0};
        const auto delta = target - a.position;
        const auto result = avoid_collisions(a, normalized(delta) * std::min(2.f, length(delta) * 2),
                                             std::span<const AvoidanceAgent>(&b, 1), 1.f / 60, settings);
        a.velocity = result.velocity;
        a.position += a.velocity / 60;
        b.position += b.velocity / 60;
        gap = std::min(gap, length(a.position - b.position) - a.radius - b.radius);
    }
    CHECK(gap >= -0.001f);
    CHECK(length(a.position - Vec2{6, 0}) < 0.3f);
}

TEST_CASE("circular avoidance is unchanged by a consistent seconds to ticks conversion") {
    const auto results = same_encounter_in_seconds_and_ticks(Footprint::circle);
    CHECK(length(results.ticks.velocity * 60 - results.seconds.velocity) < 0.001f);
    CHECK(length(results.ticks.acceleration * 3600 - results.seconds.acceleration) < 0.01f);
}

TEST_CASE("rectangular avoidance is unchanged by a consistent seconds to ticks conversion") {
    const auto results = same_encounter_in_seconds_and_ticks(Footprint::rectangle);
    CHECK(length(results.ticks.velocity * 60 - results.seconds.velocity) < 0.001f);
    CHECK(length(results.ticks.acceleration * 3600 - results.seconds.acceleration) < 0.01f);
}

TEST_CASE("rectangles pass a valid gap inside their circular envelopes") {
    const AvoidanceAgent a{0, {}, {}, 1, true, {0.5f, 0.1f}};
    const AvoidanceAgent b{1, {0, 0.25f}, {}, 1, false, {0.5f, 0.1f}};
    const auto result = avoid_collisions(a, {2, 0}, {&b, 1}, 1, rectangle_settings());
    CHECK(result.feasible);
    CHECK(length(result.velocity - Vec2{2, 0}) < 0.00001f);
}

TEST_CASE("corrected rectangle velocities never enter the swept Minkowski box") {
    // Independently check corrected velocities against the swept Minkowski box,
    // including all cutoff edges, tangent rays and corner projection regions.
    const auto settings = rectangle_settings();
    AvoidanceAgent a{0, {}, {}, 1, true, {0.6f, 0.15f}};
    AvoidanceAgent b{1, {}, {0.3f, -0.7f}, 1, false, {0.2f, 0.7f}};
    for (int direction = 0; direction < 24; ++direction) {
        b.position = rotated({3, 0}, static_cast<float>(direction) * 6.283185307f / 24);
        for (int heading = 0; heading < 24; ++heading) {
            CAPTURE(direction);
            CAPTURE(heading);
            a.velocity = rotated({2, 0}, static_cast<float>(heading) * 6.283185307f / 24);
            const auto result = avoid_collisions(a, a.velocity, {&b, 1}, 1, settings);
            const auto inflated = a.half_extents + b.half_extents + Vec2{0.0199f, 0.0199f};
            CHECK(result.feasible);
            CHECK_FALSE(
                enters_rectangle((result.velocity - b.velocity) * settings.horizon, b.position, inflated));
        }
    }
}

TEST_CASE("overlapping rectangles report infeasibility within the acceleration limit") {
    const auto pair = overlapping_rectangles();
    const auto result = avoid_collisions(pair.a, {}, {&pair.b, 1}, 0.1f, pair.settings);
    CHECK_FALSE(result.feasible);
    CHECK(finite(result.velocity));
    CHECK(length(result.acceleration) <= 0.10001f);
}

TEST_CASE("symmetric overlapping rectangles recover in opposite directions") {
    auto pair = overlapping_rectangles();
    pair.b.cooperative = true;
    for (Vec2 offset : {Vec2{}, Vec2{0.1f, 0}}) {
        INFO("offset x ", offset.x);
        pair.b.position = offset;
        const auto first = avoid_collisions(pair.a, {}, {&pair.b, 1}, 0.1f, pair.settings);
        const auto second = avoid_collisions(pair.b, {}, {&pair.a, 1}, 0.1f, pair.settings);
        CHECK(length(first.velocity + second.velocity) < 0.00001f);
        CHECK(length(first.velocity) > 0);
    }
}

TEST_CASE("partial rectangle extents are rejected") {
    auto pair = overlapping_rectangles();
    pair.b.cooperative = true;
    pair.b.position = {0.1f, 0};
    pair.b.half_extents = {1, 0};
    CHECK_THROWS_AS(avoid_collisions(pair.a, {}, {&pair.b, 1}, 1, pair.settings), std::invalid_argument);
}

TEST_CASE("a rectangle with an unsafe mixed-pair radius is rejected") {
    auto pair = overlapping_rectangles();
    pair.b.cooperative = true;
    pair.b.position = {0.1f, 0};
    pair.b.half_extents = {2, 2};
    CHECK_THROWS_AS(avoid_collisions(pair.a, {}, {&pair.b, 1}, 1, pair.settings), std::invalid_argument);
}

TEST_CASE("two rectangular agents cross a ring without overlap and stop at their goals") {
    check_crowd(ring(2, Footprint::rectangle));
}

TEST_CASE("four rectangular agents cross a ring without overlap and stop at their goals") {
    check_crowd(ring(4, Footprint::rectangle));
}

TEST_CASE("eight rectangular agents cross a ring without overlap and stop at their goals") {
    check_crowd(ring(8, Footprint::rectangle));
}

TEST_CASE("two circular agents cross a ring without overlap and stop at their goals") {
    check_crowd(ring(2, Footprint::circle));
}

TEST_CASE("four circular agents cross a ring without overlap and stop at their goals") {
    check_crowd(ring(4, Footprint::circle));
}

TEST_CASE("an agent passes a parked noncooperative body without overlap and stops at its goal") {
    check_crowd(parked_crossing());
}

TEST_CASE("eight circular agents cross a ring without overlap and stop at their goals") {
    check_crowd(ring(8, Footprint::circle));
}

TEST_CASE("sixteen circular agents cross a ring without overlap and stop at their goals") {
    check_crowd(ring(16, Footprint::circle));
}
