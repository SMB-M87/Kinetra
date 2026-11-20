// Contract tests for weighted grid planning and priority steering
// (include/kinetra/simulation/weighted_grid.h, include/kinetra/simulation/steering.h): weighted detours,
// blocked corners, footprint clearance, merged borders, safety priority and wall braking.
#include <kinetra/simulation/steering.h>
#include <kinetra/simulation/weighted_grid.h>

#include <doctest/doctest.h>

#include <cmath>
#include <span>
#include <stdexcept>
#include <vector>

using namespace kinetra;

namespace {
/// Five by three grid of 10 by 10 cells, numbered row by row.
WeightedGrid corridor_grid() {
    return WeightedGrid({50, 30}, {10, 10});
}

/// Weights for corridor_grid() that make the middle cell 7 expensive.
std::vector<float> congestion() {
    std::vector<float> weights(15);
    weights[7] = 100;
    return weights;
}

/// corridor_grid() with congestion() weights and cells 1 and 5 blocked, so the corner
/// between cells 0 and 6 is closed.
WeightedGrid blocked_corridor() {
    auto grid = corridor_grid();
    grid.weights(congestion());
    grid.block(1);
    grid.block(5);
    return grid;
}

/// Five by five grid of 10 by 10 cells with the center cell 12 blocked.
WeightedGrid footprint_grid() {
    WeightedGrid grid({50, 50}, {10, 10});
    grid.block(12);
    return grid;
}

/// Half extents of the parking footprint planned through footprint_grid().
constexpr Vec2 footprint{2, 2};

/// A 10 by 10 box at (20, 20) moving right at speed 2.
Body cruiser() {
    Body body;
    body.shape = Shape::box;
    body.half_extents = {5, 5};
    body.position = {20, 20};
    body.velocity = {2, 0};
    return body;
}

/// Default priority steering with a 10-unit arrival radius.
PrioritySteering steering_settings() {
    PrioritySteering settings;
    settings.arrival_radius = 10;
    return settings;
}

/// cruiser() placed at `position` with `velocity`.
Body cruiser_at(Vec2 position, Vec2 velocity) {
    auto body = cruiser();
    body.position = position;
    body.velocity = velocity;
    return body;
}

/// Vertical wall along x = 0.
constexpr Segment wall{{0, -100}, {0, 100}};
} // namespace

TEST_CASE("a free weighted path ends exactly at the goal") {
    const auto route = corridor_grid().path({5, 15}, {45, 15});
    REQUIRE(route.size() == 4u);
    CHECK(distance(route.back(), {45, 15}) == 0);
}

TEST_CASE("congestion weights route around an expensive cell") {
    auto grid = corridor_grid();
    grid.weights(congestion());
    for (auto point : grid.path({5, 15}, {45, 15}))
        CHECK(grid.cell(point) != 7);
}

TEST_CASE("a diagonal step does not cut a blocked corner") {
    CHECK(blocked_corridor().path({5, 5}, {15, 15}).empty());
}

TEST_CASE("a start outside the grid yields no path") {
    CHECK(blocked_corridor().path({-1, 5}, {15, 15}).empty());
}

TEST_CASE("blocked cells produce free-space borders") {
    CHECK_FALSE(blocked_corridor().borders().empty());
}

TEST_CASE("invalid weights are rejected without partially mutating the grid") {
    auto grid = blocked_corridor();
    auto weights = congestion();
    weights[0] = -1;
    CHECK_THROWS_AS(grid.weights(weights), std::invalid_argument);
    CHECK(grid.weight(0) == 0);
}

TEST_CASE("straight walls have no raster seams") {
    const WeightedGrid unblocked({50, 50}, {10, 10});
    CHECK(unblocked.borders().size() == 4u);
}

TEST_CASE("a point route passes a blocked cell that a footprint cannot") {
    CHECK(footprint_grid().path({5, 5}, {19, 25}).size() > 0u);
}

TEST_CASE("a parking footprint may not overlap a blocked neighboring cell") {
    CHECK(footprint_grid().path({5, 5}, {19, 25}, footprint).empty());
}

TEST_CASE("a parking footprint may not cross the outer boundary") {
    CHECK(footprint_grid().path({5, 5}, {1, 35}, footprint).empty());
}

TEST_CASE("a swept footprint does not cut a blocked corner") {
    CHECK_FALSE(footprint_grid().clear({15, 15}, {35, 35}, footprint));
}

TEST_CASE("a wall-tangent footprint sweep is clear") {
    CHECK(footprint_grid().clear({18, 15}, {18, 35}, footprint));
}

TEST_CASE("the footprint planner detours along clear links") {
    const auto grid = footprint_grid();
    const auto safe = grid.path({5, 25}, {45, 25}, footprint);
    REQUIRE_FALSE(safe.empty());
    Vec2 previous{5, 25};
    for (auto point : safe) {
        CHECK(grid.clear(previous, point, footprint));
        previous = point;
    }
}

TEST_CASE("collinear obstacle boundaries are merged") {
    auto grid = footprint_grid();
    grid.block(7);
    CHECK(grid.borders().size() == 8u);
}

TEST_CASE("cruising at speed does not accelerate") {
    const auto forces = priority_steer(cruiser(), {100, 20}, {}, {}, steering_settings());
    CHECK(length(forces.acceleration) == 0);
}

TEST_CASE("arrival at the target brakes") {
    const auto agent = cruiser();
    CHECK(priority_steer(agent, agent.position, {}, {}, steering_settings()).acceleration.x < 0);
}

TEST_CASE("safety takes priority over seek within the acceleration limit") {
    const auto settings = steering_settings();
    const auto neighbor = cruiser_at({28, 20}, {});
    const Segment edge{{25, 0}, {25, 100}};
    const auto forces =
        priority_steer(cruiser(), {100, 20}, std::span(&neighbor, 1), std::span(&edge, 1), settings);
    CHECK_MESSAGE(forces.border.x <= 0, "Safety priority lost to seek");
    CHECK_MESSAGE(forces.collision.x < 0, "Safety priority lost to seek");
    CHECK_MESSAGE(forces.acceleration.x < 0, "Safety priority lost to seek");
    CHECK_MESSAGE(length(forces.acceleration) <= settings.max_acceleration + 1e-6f,
                  "Acceleration limit exceeded");
}

TEST_CASE("wall safety keeps tangential steering") {
    const auto along = constrain_borders(cruiser_at({5, 20}, {0, .2f}), {-.2f, .1f}, std::span(&wall, 1),
                                         steering_settings());
    CHECK(along.x == 0);
    CHECK(std::abs(along.y - .1f) < 1e-6f);
}

TEST_CASE("the wall reaction is bounded and continuous across contact") {
    const auto settings = steering_settings();
    auto sliding = cruiser_at({5, 20}, {0, .2f});
    Vec2 previous{};
    bool first = true;
    for (float gap : {-.01f, -.001f, 0.f, .001f, .01f}) {
        CAPTURE(gap);
        sliding.position.x = 5 + gap;
        const auto value = constrain_borders(sliding, {-.2f, .1f}, std::span(&wall, 1), settings);
        CHECK_MESSAGE(finite(value), "Wall reaction exceeded acceleration bound");
        CHECK_MESSAGE(length(value) <= settings.max_acceleration + 1e-6f,
                      "Wall reaction exceeded acceleration bound");
        if (!first)
            CHECK_MESSAGE(distance(previous, value) < .01f, "Wall reaction jumps at contact");
        first = false;
        previous = value;
    }
}

TEST_CASE("a parked touching body is not repelled") {
    CHECK(length(constrain_borders(cruiser_at({5, 20}, {}), {}, std::span(&wall, 1), steering_settings())) ==
          0);
}

TEST_CASE("wall braking converges to a close parking goal without crossing the wall") {
    const auto settings = steering_settings();
    auto sliding = cruiser_at({25, 20}, {-2, 0});
    for (int tick = 0; tick < 400; ++tick) {
        const Vec2 desired =
            limited(Vec2{5.25f - sliding.position.x, .1f} - sliding.velocity, settings.max_acceleration);
        const auto acceleration = constrain_borders(sliding, desired, std::span(&wall, 1), settings);
        sliding.velocity = limited(sliding.velocity + acceleration, settings.max_speed);
        sliding.position += sliding.velocity;
        REQUIRE_MESSAGE(sliding.position.x >= 5 - 1e-5f, "Normal braking crossed the wall");
        REQUIRE_MESSAGE(length(acceleration) <= settings.max_acceleration + 1e-5f,
                        "Normal braking exceeded the acceleration budget");
    }
    CHECK_MESSAGE(std::abs(sliding.position.x - 5.25f) < 1e-3f,
                  "Wall braking prevented convergence to a close parking goal");
}

TEST_CASE("a corner approach brakes on both axes") {
    const Segment corner[] = {{{0, -100}, {0, 100}}, {{-100, 0}, {100, 0}}};
    const auto constrained = constrain_borders(cruiser_at({5.1f, 5.1f}, {-.05f, -.05f}), {-.2f, -.2f}, corner,
                                               steering_settings());
    CHECK(constrained.x >= 0);
    CHECK(constrained.y >= 0);
}

TEST_CASE("a body sliding along a wall passes the convex corner where the wall ends") {
    // A step: the floor y = 0 ends at x = 0, where the wall drops away along x = 0. The box
    // rides flush on the floor (its bottom at y = 0) toward a goal below the step, so its
    // corner meets the end of the drop exactly; that touch must not block the slide.
    const Segment step[] = {{{-100, 0}, {0, 0}}, {{0, 0}, {0, 100}}};
    const auto settings = steering_settings();
    // One unit before the corner, close enough that a wall ahead would brake.
    const auto near = constrain_borders(cruiser_at({-6, -5}, {2, 0}), {.2f, .2f}, step, settings);
    CHECK_MESSAGE(near.x == doctest::Approx(.2f), "The end of the drop braked the slide");
    CHECK_MESSAGE(near.y <= 0, "The slide pushed into the floor");
    auto sliding = cruiser_at({-20, -5}, {2, 0});
    const Vec2 goal{30, 40};
    for (int tick = 0; tick < 200; ++tick) {
        const Vec2 desired = limited(goal - sliding.position - sliding.velocity, settings.max_acceleration);
        sliding.velocity = limited(sliding.velocity + constrain_borders(sliding, desired, step, settings),
                                   settings.max_speed);
        sliding.position += sliding.velocity;
        REQUIRE_MESSAGE((sliding.position.y <= -5 + 1e-4f || sliding.position.x >= 5 - 1e-4f),
                        "The box cut the corner of the step");
    }
    CHECK_MESSAGE(distance(sliding.position, goal) < 1, "The box stuck at the corner of the step");
}

TEST_CASE("an approaching collision is predicted") {
    const auto neighbor = cruiser_at({40, 20}, {-2, 0});
    const auto forces =
        priority_steer(cruiser(), {100, 20}, std::span(&neighbor, 1), {}, steering_settings());
    CHECK(forces.predictive.x < 0);
    CHECK(forces.acceleration.x < 0);
}

TEST_CASE("coincident bodies get a finite collision response") {
    const auto agent = cruiser();
    const auto neighbor = cruiser_at(agent.position, {-2, 0});
    const auto forces = priority_steer(agent, {100, 20}, std::span(&neighbor, 1), {}, steering_settings());
    CHECK(finite(forces.acceleration));
    CHECK(length(forces.collision) > 0);
}

TEST_CASE("tangent steering stays finite and within its acceleration limit") {
    const auto agent = cruiser();
    const auto neighbor = cruiser_at(agent.position, {-2, 0});
    const auto tangent = tangent_steer(agent, {100, 20}, std::span(&neighbor, 1));
    CHECK_MESSAGE(finite(tangent), "Invalid tangent acceleration");
    CHECK_MESSAGE(length(tangent) <= .600001f, "Invalid tangent acceleration");
    CHECK_MESSAGE(length(tangent_steer(agent, agent.position, {})) <= .600001f,
                  "Tangent arrival acceleration exceeded limit");
}

TEST_CASE("tangent steering rejects an unbounded horizon") {
    CHECK_THROWS_AS(tangent_steer(cruiser(), {100, 20}, {}, 2, .6f, 65), std::invalid_argument);
}
