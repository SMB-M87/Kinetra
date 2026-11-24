// Contract tests for the core simulation: collision geometry, world stepping and contact
// response (physics.h), spatial and collision grids, steering, the fixed clock and camera,
// invalid input, and a 400-body stability run.
#include <kinetra/runtime/timing.h>
#include <kinetra/simulation/collision_grid.h>
#include <kinetra/simulation/steering.h>
#include <kinetra/view/camera.h>

#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace kinetra;

namespace {
/// Whether `actual` lies within `tolerance` of `expected`, inclusive.
bool near(float actual, float expected, float tolerance = 0.001f) {
    return std::abs(actual - expected) <= tolerance;
}

/// Circle at `p` with `radius`.
Body circle(Vec2 p, float radius = 0.5f) {
    Body b;
    b.position = p;
    b.radius = radius;
    return b;
}

/// Box at `p` with half extents `size`, rotated by `angle` radians.
Body box(Vec2 p, Vec2 size, float angle = 0) {
    Body b;
    b.shape = Shape::box;
    b.position = p;
    b.half_extents = size;
    b.angle = angle;
    return b;
}

/// 400 boxes from a fixed seed, centered in [-40, 40]^2 with half extents in [0.01, 5].
std::vector<Aabb> random_boxes() {
    std::mt19937 rng(17);
    std::uniform_real_distribution<float> coordinate(-40, 40), size(0.01f, 5);
    std::vector<Aabb> boxes;
    for (int i = 0; i < 400; ++i) {
        const Vec2 center{coordinate(rng), coordinate(rng)}, extent{size(rng), size(rng)};
        boxes.push_back({center - extent, center + extent});
    }
    return boxes;
}

/// Fingerprint of a mixed world over 360 steps: fixed walls, boxes and polygons interleaved by
/// index with moving circles, boxes and a polygon, set out symmetrically so that equal-time
/// events occur. Hashes every body's position, velocity and angles and candidate_pairs().
std::uint64_t mixed_world_fingerprint() {
    World world(2);
    world.gravity = {0, -10};
    std::mt19937 rng(29);
    std::uniform_real_distribution<float> unit(-1, 1);
    for (int i = 0; i < 24; ++i) {
        const float x = float(i % 6) * 3 - 7.5f, y = float(i / 6) * 3 - 4.5f;
        Body fixed;
        fixed.motion = Motion::fixed;
        fixed.position = {x, y};
        if (i % 3 == 0) {
            fixed.shape = Shape::box;
            fixed.half_extents = {0.8f, 0.3f};
            fixed.angle = 0.25f * float(i % 4);
        } else if (i % 3 == 1) {
            fixed = regular_polygon(5 + i % 3, 0.6f);
            fixed.motion = Motion::fixed;
            fixed.position = {x, y};
        } else {
            fixed.radius = 0.4f;
        }
        world.add(fixed);
        Body mover;
        mover.position = {x + 1.5f, y + 1.5f};
        mover.velocity = {float(i % 2 ? 2 : -2), unit(rng)};
        mover.motion = i % 4 == 0 ? Motion::controlled : Motion::dynamic;
        if (i % 5 == 0) {
            mover.shape = Shape::box;
            mover.half_extents = {0.4f, 0.4f};
        } else if (i % 7 == 0) {
            mover = regular_polygon(6, 0.45f);
            mover.position = {x + 1.5f, y + 1.5f};
            mover.velocity = {-2, 0};
        } else {
            mover.radius = 0.35f;
            mover.angular_velocity = unit(rng);
        }
        world.add(mover);
    }
    // A container added last, so that movers resting on it and on each other meet a fixed
    // body with a higher index than their moving neighbors.
    for (const auto &[position, extent] :
         {std::pair{Vec2{0, -7}, Vec2{10, 0.5f}}, std::pair{Vec2{-10, 0}, Vec2{0.5f, 8}},
          std::pair{Vec2{10, 0}, Vec2{0.5f, 8}}}) {
        Body wall;
        wall.motion = Motion::fixed;
        wall.shape = Shape::box;
        wall.position = position;
        wall.half_extents = extent;
        world.add(wall);
    }
    std::uint64_t hash = 1469598103934665603ull;
    auto mix = [&](float value) {
        std::uint32_t bits;
        std::memcpy(&bits, &value, sizeof bits);
        for (int k = 0; k < 4; ++k) {
            hash ^= (bits >> (8 * k)) & 0xff;
            hash *= 1099511628211ull;
        }
    };
    for (int step = 0; step < 360; ++step) {
        world.step(1.f / 60);
        for (const auto &b : world.bodies())
            for (float v :
                 {b.position.x, b.position.y, b.velocity.x, b.velocity.y, b.angle, b.angular_velocity})
                mix(v);
        mix(float(world.candidate_pairs()));
    }
    return hash;
}

/// Angular velocity of a box mover with `motion` after one step into a fixed box off its center.
float spin_after_off_center_hit(Motion motion) {
    World world;
    Body obstacle = box({0, 0}, {0.5f, 0.5f});
    obstacle.motion = Motion::fixed;
    Body mover = box({-1.05f, 0.65f}, {0.6f, 0.5f});
    mover.velocity = {4, 0};
    mover.motion = motion;
    mover.friction = obstacle.friction = 0;
    world.add(obstacle);
    const auto id = world.add(mover);
    world.step(1.f / 120);
    return world.body(id).angular_velocity;
}

/// A frictionless controlled circle driving into a resting dynamic circle.
struct PushingPair {
    World world;
    std::size_t pusher{}, pushed{};
};

PushingPair pushing_pair() {
    PushingPair pair;
    Body controlled = circle({-0.49f, 0});
    controlled.motion = Motion::controlled;
    controlled.velocity = {4, 0};
    Body physical = circle({0.49f, 0});
    controlled.friction = physical.friction = 0;
    pair.pusher = pair.world.add(controlled);
    pair.pushed = pair.world.add(physical);
    return pair;
}

/// Steps `world` 120 times at 120 Hz.
void run_one_second(World &world) {
    for (int i = 0; i < 120; ++i)
        world.step(1.f / 120);
}

/// A 4 by 2 box at the origin moving right at speed 1.
Body long_mover() {
    Body agent = box({0, 0}, {2, 1});
    agent.velocity = {1, 0};
    return agent;
}

/// Steering with no clearance margin, a 10-unit lookahead and rectangular clearance on.
Steering lane_settings() {
    Steering settings;
    settings.clearance = 0;
    settings.lookahead = 10;
    settings.rectangular_clearance = true;
    return settings;
}

/// Camera at (4, -3) with a 1920 by 1080 viewport and 32 pixels per unit.
Camera2D hd_camera() {
    Camera2D camera;
    camera.position = {4, -3};
    camera.viewport = {1920, 1080};
    camera.pixels_per_unit = 32;
    return camera;
}

/// Row-major 3 by 3 occupancy with only the center cell solid.
constexpr std::array<int, 9> center_cell{0, 0, 0, 0, 1, 0, 0, 0, 0};

/// 3 by 3 grid of 2-unit cells from (-3, -3): the solid cell spans [-1, 1]^2.
CollisionGrid center_cell_grid() {
    return {center_cell, 3, 3, 2, {-3, -3}};
}

/// A controlled circle of radius 0.5 driven into the solid cell of center_cell_grid().
Body grid_actor() {
    Body actor;
    actor.motion = Motion::controlled;
    actor.position = {-1.3f, 0};
    actor.velocity = {2, 0};
    actor.radius = 0.5f;
    collide_grid(actor, center_cell_grid());
    return actor;
}

/// grid_actor() relaunched into a light crate, after their contact is resolved.
struct Push {
    Body actor, crate;
};

Push pushed_crate() {
    Push push{grid_actor(), {}};
    push.actor.position = {-0.8f, 0};
    push.actor.velocity = {3, 0};
    push.crate.position = {};
    push.crate.radius = 0.5f;
    push.crate.mass = 0.5f;
    const auto contact = collide(push.actor, push.crate);
    REQUIRE(contact);
    resolve_contact(push.actor, push.crate, *contact);
    return push;
}

/// `value`, read back through a volatile so the compiler cannot fold a call on it at compile
/// time: folding rounds correctly whatever the C library does, so a test on constant
/// arguments would check the compiler instead of the library.
float at_run_time(float value) {
    volatile float copy = value;
    return float{copy};
}
} // namespace

// The inputs are cases where glibc before 2.41 rounds atan2f and asinf to the wrong neighbor
// of the exact result (found on Ubuntu 24.04, glibc 2.39). The expected values are the
// correctly rounded floats, which glibc 2.41 and later also return. The inputs pass through
// at_run_time() so the C library computes them.
TEST_CASE("atan2 and asin return the correctly rounded float on every C library") {
    CHECK(kinetra::atan2(at_run_time(0x1.05ebb8p+5f), at_run_time(-0x1.ed77fp+3f)) == 0x1.0167eap+1f);
    CHECK(kinetra::atan2(at_run_time(-0x1.5adep+4f), at_run_time(0x1.5483ap+6f)) == -0x1.feb26ap-3f);
    CHECK(kinetra::asin(at_run_time(-0x1.a1be34p-1f)) == -0x1.e898e2p-1f);
    CHECK(kinetra::asin(at_run_time(-0x1.2e06f8p-1f)) == -0x1.43096ap-1f);
}

TEST_CASE("atan2 and asin keep the standard quadrants and special values") {
    constexpr float pi = 3.14159265358979f, infinity = std::numeric_limits<float>::infinity();
    const auto run_atan2 = [](float y, float x) { return kinetra::atan2(at_run_time(y), at_run_time(x)); };
    const auto run_asin = [](float x) { return kinetra::asin(at_run_time(x)); };
    CHECK(run_atan2(0.f, 1.f) == 0.f);
    CHECK(run_atan2(1.f, 0.f) == doctest::Approx(pi / 2));
    CHECK(run_atan2(0.f, -1.f) == pi);
    CHECK(run_atan2(-0.f, -1.f) == -pi);
    CHECK(std::signbit(run_atan2(-0.f, 1.f)));
    CHECK(run_atan2(-1.f, -1.f) == doctest::Approx(-3 * pi / 4));
    CHECK(run_atan2(infinity, infinity) == doctest::Approx(pi / 4));
    CHECK(std::isnan(run_atan2(std::nanf(""), 1.f)));
    CHECK(run_asin(1.f) == doctest::Approx(pi / 2));
    CHECK(std::signbit(run_asin(-0.f)));
    CHECK(std::isnan(run_asin(1.5f)));
}

TEST_CASE("touching circles collide with zero penetration along the center line") {
    const auto c = collide(circle({0, 0}), circle({1, 0}));
    REQUIRE(c);
    CHECK(near(c->penetration, 0));
    CHECK(near(c->normal.x, 1));
}

TEST_CASE("separated circles do not collide") {
    CHECK_FALSE(collide(circle({0, 0}), circle({1.1f, 0})));
}

TEST_CASE("coincident circles get a finite contact normal") {
    const auto c = collide(circle({0, 0}), circle({0, 0}));
    REQUIRE(c);
    CHECK(finite(c->normal));
}

TEST_CASE("a rotated box collides by its oriented shape") {
    const Body rotated_box = box({0, 0}, {2, 0.25f}, 0.78539816f);
    CHECK_MESSAGE(collide(rotated_box, circle({1.2f, 1.2f})).has_value(), "Rotated box missed");
    CHECK_MESSAGE(!collide(rotated_box, circle({1.2f, -1.2f}, 0.2f)), "Rotated box false positive");
}

TEST_CASE("a circle inside a box gets a containment contact") {
    const auto inside = collide(circle({0, 0}), box({0, 0}, {2, 1}));
    REQUIRE(inside);
    CHECK(inside->penetration > 1);
}

TEST_CASE("a box corner contact has a diagonal normal that flips with argument order") {
    const Body corner = circle({2.3f, 1.3f});
    const auto c = collide(corner, box({0, 0}, {2, 1}));
    REQUIRE(c);
    CHECK(c->normal.x < 0);
    CHECK(c->normal.y < 0);
    const auto reverse = collide(box({0, 0}, {2, 1}), corner);
    REQUIRE(reverse);
    CHECK(near(dot(c->normal, reverse->normal), -1));
}

TEST_CASE("oriented boxes collide only when they overlap") {
    const Body rotated_box = box({0, 0}, {2, 0.25f}, 0.78539816f);
    CHECK_MESSAGE(collide(rotated_box, box({0.5f, 0.5f}, {0.5f, 0.5f}, -0.3f)).has_value(),
                  "OBB contact missed");
    CHECK_MESSAGE(!collide(rotated_box, box({4, -3}, {0.5f, 0.5f}, -0.3f)), "OBB false positive");
}

TEST_CASE("spatial grid queries match an exhaustive search") {
    const auto boxes = random_boxes();
    SpatialGrid grid(3);
    grid.rebuild(boxes);
    for (const auto &area : boxes) {
        std::vector<std::size_t> expected;
        for (std::size_t j = 0; j < boxes.size(); ++j)
            if (boxes[j].overlaps(area))
                expected.push_back(j);
        CHECK(grid.query(area) == expected);
    }
}

TEST_CASE("rebuilding a spatial grid without boxes drops stale objects") {
    const auto boxes = random_boxes();
    SpatialGrid grid(3);
    grid.rebuild(boxes);
    grid.rebuild({});
    CHECK(grid.query({{-1, -1}, {1, 1}}).empty());
}

TEST_CASE("a spatial grid rebuilt over moving boxes matches an exhaustive search") {
    // Rebuilds keep the storage of cells in use; boxes that leave and return must still be found.
    const auto boxes = random_boxes();
    SpatialGrid grid(3);
    for (float shift : {0.f, 60.f, 0.f, -30.f, 0.f}) {
        CAPTURE(shift);
        auto moved = boxes;
        for (std::size_t i = 0; i < moved.size(); i += 2) {
            moved[i].min.x += shift;
            moved[i].max.x += shift;
        }
        grid.rebuild(moved);
        for (const auto &area : boxes) {
            std::vector<std::size_t> expected;
            for (std::size_t j = 0; j < moved.size(); ++j)
                if (moved[j].overlaps(area))
                    expected.push_back(j);
            CHECK(grid.query(area) == expected);
        }
    }
}

TEST_CASE("a spatial grid gives the same results over a compact and a scattered set") {
    // A compact set fills a dense array of cells; one far box makes the grid fall back to its
    // hash map. Both must match an exhaustive search, also through the caller's buffer.
    auto compact = random_boxes();
    auto scattered = compact;
    scattered.push_back({{1000000, -1}, {1000001, 1}});
    for (const auto *boxes : {&compact, &scattered}) {
        CAPTURE(boxes->size());
        SpatialGrid grid(3);
        grid.rebuild(*boxes);
        std::vector<std::size_t> buffer{99};
        auto areas = *boxes;
        areas.push_back({{-300, -300}, {300, 300}});
        areas.push_back({{999999, -5}, {1000002, 0}});
        areas.push_back({{500, 500}, {501, 501}});
        for (const auto &area : areas) {
            std::vector<std::size_t> expected;
            for (std::size_t j = 0; j < boxes->size(); ++j)
                if ((*boxes)[j].overlaps(area))
                    expected.push_back(j);
            CHECK(grid.query(area) == expected);
            grid.query(area, buffer);
            CHECK(buffer == expected);
        }
    }
}

TEST_CASE("a rejected spatial query leaves the caller's buffer unchanged") {
    SpatialGrid grid(1);
    grid.rebuild(random_boxes());
    std::vector<std::size_t> buffer{1, 2, 3};
    CHECK_THROWS_AS(grid.query({{0, 0}, {1000, 1000}}, buffer), std::invalid_argument);
    CHECK_THROWS_AS(grid.query({{1, 0}, {0, 1}}, buffer), std::invalid_argument);
    CHECK(buffer == std::vector<std::size_t>{1, 2, 3});
}

TEST_CASE("an elastic circle collision conserves momentum and energy") {
    World world;
    Body a = circle({-0.48f, 0}), b = circle({0.48f, 0});
    a.velocity = {2, 0};
    b.velocity = {-1, 0};
    a.restitution = b.restitution = 1;
    a.friction = b.friction = 0;
    const auto ai = world.add(a), bi = world.add(b);
    world.step(1.f / 120);
    CHECK(near(world.body(ai).velocity.x, -1));
    CHECK(near(world.body(bi).velocity.x, 2));
    CHECK(near(world.body(ai).velocity.x + world.body(bi).velocity.x, 1));
    CHECK(near(dot(world.body(ai).velocity, world.body(ai).velocity) +
                   dot(world.body(bi).velocity, world.body(bi).velocity),
               5));
    CHECK_MESSAGE(length(world.body(ai).position - world.body(bi).position) > 0.99f, "Penetration remains");
}

TEST_CASE("a controlled body slides along a fixed wall") {
    World world;
    Body wall = box({0, 0}, {0.5f, 10});
    wall.motion = Motion::fixed;
    Body mover = circle({-1.1f, 0});
    mover.motion = Motion::controlled;
    mover.velocity = {3, 2};
    mover.friction = wall.friction = 0;
    const auto wi = world.add(wall), id = world.add(mover);
    for (int i = 0; i < 60; ++i)
        world.step(1.f / 120);
    CHECK(near(world.body(id).velocity.x, 0));
    CHECK(near(world.body(id).velocity.y, 2));
    CHECK(near(world.body(id).angular_velocity, 0));
    CHECK(world.body(id).position.x < -0.99f);
    CHECK(world.body(id).position.y > 0.9f);
    CHECK(near(world.body(wi).position.x, 0));
}

TEST_CASE("an off-center collision rotates a dynamic body") {
    CHECK(std::abs(spin_after_off_center_hit(Motion::dynamic)) > 0.1f);
}

TEST_CASE("an off-center collision does not rotate a controlled body") {
    CHECK(near(spin_after_off_center_hit(Motion::controlled), 0));
}

TEST_CASE("a controlled body pushes a dynamic body without bouncing") {
    auto pair = pushing_pair();
    run_one_second(pair.world);
    CHECK_MESSAGE(pair.world.body(pair.pushed).velocity.x > 1, "Controlled body failed to push dynamic body");
    CHECK_MESSAGE(pair.world.body(pair.pusher).velocity.x >= 0, "Controlled collision bounced");
}

TEST_CASE("a copied world replays bit-identically") {
    auto pair = pushing_pair();
    World replay = pair.world;
    for (int i = 0; i < 120; ++i) {
        pair.world.step(1.f / 120);
        replay.step(1.f / 120);
    }
    CHECK(near(pair.world.body(pair.pusher).position.x, replay.body(pair.pusher).position.x, 0));
    CHECK(near(pair.world.body(pair.pushed).position.x, replay.body(pair.pushed).position.x, 0));
}

TEST_CASE("clearing a world drops its bodies and candidate pairs") {
    auto pair = pushing_pair();
    run_one_second(pair.world);
    pair.world.clear();
    CHECK(pair.world.bodies().empty());
    CHECK(pair.world.candidate_pairs() == 0u);
}

TEST_CASE("a sliding box comes to rest on a floor through friction") {
    World world;
    world.gravity = {0, -10};
    Body floor = box({0, -1}, {20, 0.5f});
    floor.motion = Motion::fixed;
    floor.friction = 1;
    Body resting = box({0, 2}, {0.5f, 0.5f});
    resting.friction = 1;
    resting.restitution = 0;
    resting.velocity = {3, 0};
    world.add(floor);
    const auto id = world.add(resting);
    for (int i = 0; i < 1200; ++i)
        world.step(1.f / 120);
    const auto &result = world.body(id);
    CHECK_MESSAGE(finite(result.position), "Body did not rest on floor");
    CHECK_MESSAGE(result.position.y > -0.03f, "Body did not rest on floor");
    CHECK_MESSAGE(result.position.y < 0.1f, "Body did not rest on floor");
    CHECK_MESSAGE(std::abs(result.velocity.x) < 0.1f, "Friction/resting contact unstable");
    CHECK_MESSAGE(std::abs(result.velocity.y) < 0.1f, "Friction/resting contact unstable");
}

TEST_CASE("gravity accelerates a free body") {
    World world;
    world.gravity = {0, -10};
    const auto id = world.add(circle({0, 0}));
    run_one_second(world);
    CHECK(near(world.body(id).velocity.y, -10, 0.001f));
    CHECK(near(world.body(id).position.y, -5, 0.03f));
}

TEST_CASE("a circle contained in a fixed box is pushed out") {
    World inside;
    Body container = box({0, 0}, {2, 1});
    container.motion = Motion::fixed;
    inside.add(container);
    const auto ci = inside.add(circle({0, 0}));
    for (int i = 0; i < 4; ++i)
        inside.step(1.f / 120);
    CHECK(std::abs(inside.body(ci).position.y) > 1.49f);
}

TEST_CASE("steering arrives and brakes within the acceleration limit") {
    Body agent = circle({0, 0});
    const Steering settings;
    const Vec2 target{10, 0};
    for (int i = 0; i < 1200; ++i) {
        const Vec2 acceleration = steer(agent, target, {}, settings);
        REQUIRE_MESSAGE(length(acceleration) <= settings.max_acceleration + 0.001f, "Unbounded steering");
        agent.velocity += acceleration * (1.f / 120);
        agent.position += agent.velocity * (1.f / 120);
    }
    CHECK(length(agent.position - target) < 0.2f);
    CHECK(length(agent.velocity) < 0.05f);
}

TEST_CASE("an obstacle ahead changes the steering direction") {
    Body agent = circle({0, 0});
    agent.velocity = {3, 0};
    const std::array<Body, 1> obstacles{box({2, 0}, {0.4f, 1}, 0.3f)};
    const Vec2 avoided = steer(agent, {10, 0}, obstacles, Steering{});
    CHECK(std::abs(avoided.y) > 0.1f);
    CHECK(avoided.x < 0);
}

TEST_CASE("integrated steering passes a single obstacle without colliding") {
    // Integrated avoidance must pass a single obstacle without relying on collision correction.
    const Steering settings;
    Body agent = circle({-5, 0}, 0.3f);
    const Vec2 target{5, 0};
    const std::array<Body, 1> barrier{box({0, 0}, {0.4f, 1})};
    for (int i = 0; i < 2400; ++i) {
        agent.velocity += steer(agent, target, barrier, settings) * (1.f / 120);
        agent.position += agent.velocity * (1.f / 120);
        const auto c = collide(agent, barrier[0]);
        REQUIRE_MESSAGE((!c || c->penetration < 0.01f), "Steering ran into obstacle");
    }
    CHECK_MESSAGE(length(agent.position - target) < 0.3f, "Steering did not get around obstacle");
}

TEST_CASE("rectangular clearance allows a physically clear passage") {
    // A long rectangular mover can pass a box beside its path even when
    // its circumscribed circle would report a blocked corridor.
    const std::array<Body, 1> beside{box({4, 2.5f}, {1, 1})};
    const auto clear = steer(long_mover(), {10, 0}, beside, lane_settings());
    CHECK(std::abs(clear.y) < 0.00001f);
    CHECK(clear.x > 0);
}

TEST_CASE("rectangular clearance still avoids a blocking box") {
    const std::array<Body, 1> ahead{box({4, 0}, {1, 1})};
    CHECK(std::abs(steer(long_mover(), {10, 0}, ahead, lane_settings()).y) > 0);
}

TEST_CASE("default steering keeps the circumscribed-circle clearance") {
    const std::array<Body, 1> beside{box({4, 2.5f}, {1, 1})};
    auto settings = lane_settings();
    settings.rectangular_clearance = false;
    CHECK(std::abs(steer(long_mover(), {10, 0}, beside, settings).y) > 0);
}

TEST_CASE("rotated boxes keep conservative clearance") {
    const std::array<Body, 1> ahead{box({4, 0}, {1, 1}, 0.2f)};
    auto settings = lane_settings();
    settings.rectangular_clearance = false;
    const auto conservative = steer(long_mover(), {10, 0}, ahead, settings);
    settings.rectangular_clearance = true;
    CHECK(length(steer(long_mover(), {10, 0}, ahead, settings) - conservative) < 0.00001f);
}

TEST_CASE("separation pushes a resting agent away with bounded acceleration") {
    // At rest, personal space still pushes away; it vanishes beyond the radius.
    const Body agent = circle({0, 0}, 0.2f);
    const std::array<Body, 1> neighbor{circle({0, 1}, 0.2f)};
    Steering settings;
    settings.separation_radius = 2;
    const auto separated = steer(agent, agent.position, neighbor, settings);
    CHECK(separated.y < 0);
    CHECK(length(separated) <= settings.max_acceleration);
}

TEST_CASE("neighbors beyond the separation radius do not push") {
    const Body agent = circle({0, 0}, 0.2f);
    const std::array<Body, 1> neighbor{circle({0, 1}, 0.2f)};
    Steering settings;
    settings.separation_radius = 0.5f;
    CHECK(length(steer(agent, agent.position, neighbor, settings)) == 0);
}

TEST_CASE("a negative separation radius is rejected") {
    const Body agent = circle({0, 0}, 0.2f);
    const std::array<Body, 1> neighbor{circle({0, 1}, 0.2f)};
    Steering settings;
    settings.separation_radius = -1;
    CHECK_THROWS_AS(steer(agent, agent.position, neighbor, settings), std::invalid_argument);
}

TEST_CASE("the fixed clock ticks independently of the rendering frequency") {
    FixedClock a(0.01, 16), b(0.01, 16);
    int ticks_a = 0, ticks_b = 0;
    for (int i = 0; i < 100; ++i)
        ticks_a += a.advance(0.01);
    for (int i = 0; i < 200; ++i)
        ticks_b += b.advance(0.005);
    CHECK(ticks_a == 100);
    CHECK(ticks_b == 100);
}

TEST_CASE("the fixed clock bounds its catch-up backlog") {
    FixedClock clock(0.01, 16);
    for (int i = 0; i < 100; ++i)
        clock.advance(0.01);
    CHECK(clock.advance(10) == 16);
    CHECK(clock.interpolation() < 1);
}

TEST_CASE("camera conversions round-trip between world and screen") {
    const auto camera = hd_camera();
    const Vec2 point{-17, 11};
    const Vec2 back = camera.screen_to_world(camera.world_to_screen(point));
    CHECK(near(back.x, point.x));
    CHECK(near(back.y, point.y));
}

TEST_CASE("the camera position maps to the viewport center") {
    const auto camera = hd_camera();
    CHECK(near(camera.world_to_screen(camera.position).x, 960));
}

TEST_CASE("invalid spatial grid settings and queries are rejected") {
    CHECK_THROWS_AS(SpatialGrid(0), std::invalid_argument);
    SpatialGrid grid;
    CHECK_THROWS_AS(grid.query({{2, 0}, {1, 1}}), std::invalid_argument);
    CHECK_THROWS_AS(grid.query({{-1e30f, 0}, {1e30f, 1}}), std::invalid_argument);
}

TEST_CASE("invalid world steps are rejected") {
    World world;
    CHECK_THROWS_AS(world.step(0), std::invalid_argument);
    CHECK_THROWS_AS(world.step(1), std::invalid_argument);
}

TEST_CASE("invalid bodies are rejected when added to a world") {
    World world;
    Body b;
    b.mass = 0;
    CHECK_THROWS_AS(world.add(b), std::invalid_argument);
    b = Body{};
    b.position.x = std::numeric_limits<float>::quiet_NaN();
    CHECK_THROWS_AS(world.add(b), std::invalid_argument);
}

TEST_CASE("a zero steering response time is rejected") {
    Steering s;
    s.response_time = 0;
    CHECK_THROWS_AS(steer(Body{}, {}, {}, s), std::invalid_argument);
}

TEST_CASE("a negative clock step is rejected") {
    CHECK_THROWS_AS(FixedClock(-1), std::invalid_argument);
}

TEST_CASE("a zero camera scale is rejected") {
    Camera2D camera;
    camera.pixels_per_unit = 0;
    CHECK_THROWS_AS(camera.world_to_screen({}), std::invalid_argument);
}

TEST_CASE("four hundred steered bodies stay finite and stable") {
    World world;
    for (int i = 0; i < 400; ++i) {
        Body b = circle({float(i % 20) * 1.8f, float(i / 20) * 1.8f}, 0.3f);
        if (i % 3 == 0) {
            b.shape = Shape::box;
            b.half_extents = {0.3f, 0.2f};
            b.angle = static_cast<float>(i) * 0.1f;
        }
        b.motion = i % 2 ? Motion::controlled : Motion::dynamic;
        world.add(b);
    }
    SpatialGrid neighbors(4);
    std::vector<Aabb> boxes;
    std::vector<Body> nearby;
    for (int tick = 0; tick < 240; ++tick) {
        boxes.clear();
        for (const auto &b : world.bodies())
            boxes.push_back(bounds(b));
        neighbors.rebuild(boxes);
        for (std::size_t i = 0; i < world.bodies().size(); ++i) {
            auto &b = world.body(i);
            nearby.clear();
            for (auto j : neighbors.query({b.position - Vec2{6, 6}, b.position + Vec2{6, 6}}))
                if (j != i)
                    nearby.push_back(world.body(j));
            b.acceleration = steer(b, {18, 18}, nearby);
        }
        world.step(1.f / 120);
        for (const auto &b : world.bodies()) {
            REQUIRE_MESSAGE(finite(b.position), "Nonfinite swarm state");
            REQUIRE_MESSAGE(finite(b.velocity), "Nonfinite swarm state");
            REQUIRE_MESSAGE(std::isfinite(b.angle), "Nonfinite swarm state");
            REQUIRE_MESSAGE(length(b.velocity) < 20, "Unstable swarm velocity");
        }
    }
}

TEST_CASE("a tiny relative projectile step does not tunnel through a circle") {
    const auto tiny_hit = segment_circle({1.0001f, 0}, {0.9999f, 0}, {}, 1);
    REQUIRE(tiny_hit);
    CHECK(*tiny_hit > 0);
    CHECK(*tiny_hit < 1);
}

TEST_CASE("a long grazing projectile hits a tangent circle") {
    CHECK(segment_circle({-10000, 25}, {10000, 25}, {}, 25).has_value());
}

TEST_CASE("a mixed world steps as it did with one grid over all bodies") {
    // The value was recorded with the broad phase before fixed bodies had an index of their
    // own (Kinetra 73adc66). The pairs' order decides ties between equal-time events and the
    // order of the correction passes, so a wrong merge of the two indexes changes it.
    CHECK(mixed_world_fingerprint() == 0x0d6bd60145972dd1ull);
}

TEST_CASE("a fixed body changed between steps acts as in a new world") {
    // World indexes fixed bodies once and must notice every change a caller makes through
    // body() or add(). A new world with the same bodies has never built an index. Each
    // change makes the wall, or a new fixed body, reach the leading mover earlier than the
    // old wall would, or makes the trailing mover, which comes first in index order and so
    // starts the query, meet the leading one after it turned fixed.
    struct Change {
        const char *name;
        void (*apply)(World &);
    };
    const Change changes[] = {
        {"moved", [](World &w) { w.body(2).position = {0.7f, 0}; }},
        {"turned", [](World &w) { w.body(2).angle = 1.5f; }},
        {"resized", [](World &w) { w.body(2).half_extents = {1.0f, 2}; }},
        {"reshaped",
         [](World &w) {
             w.body(2).shape = Shape::circle;
             w.body(2).radius = 1;
         }},
        {"released", [](World &w) { w.body(2).motion = Motion::dynamic; }},
        {"fixed", [](World &w) { w.body(1).motion = Motion::fixed; }},
        {"added",
         [](World &w) {
             Body extra = box({0.75f, 0}, {0.1f, 2});
             extra.motion = Motion::fixed;
             w.add(extra);
         }},
    };
    for (const auto &change : changes) {
        const std::string name = change.name;
        CAPTURE(name);
        World world;
        Body leading = circle({0, 0}), trailing = circle({-1.6f, 0});
        leading.velocity = {2, 0};
        trailing.velocity = {4, 0};
        Body wall = box({1.3f, 0}, {0.3f, 2});
        wall.motion = Motion::fixed;
        world.add(trailing);
        world.add(leading);
        world.add(wall);
        world.step(1.f / 120);
        change.apply(world);
        World fresh;
        for (const auto &b : world.bodies())
            fresh.add(b);
        for (int i = 0; i < 60; ++i) {
            world.step(1.f / 120);
            fresh.step(1.f / 120);
            CHECK(world.candidate_pairs() == fresh.candidate_pairs());
        }
        for (std::size_t i = 0; i < world.bodies().size(); ++i) {
            CHECK(world.body(i).position.x == fresh.body(i).position.x);
            CHECK(world.body(i).position.y == fresh.body(i).position.y);
        }
    }
}

TEST_CASE("fixed and moving contacts do not depend on insertion order") {
    // Insertion order must not drop moving/fixed pairs when fixed queries are skipped.
    for (bool fixed_first : {false, true}) {
        CAPTURE(fixed_first);
        World world;
        Body wall, mover;
        wall.motion = Motion::fixed;
        wall.position = {0, 0};
        mover.position = {-0.9f, 0};
        mover.velocity = {1, 0};
        const auto a = world.add(fixed_first ? wall : mover);
        const auto b = world.add(fixed_first ? mover : wall);
        world.step(1.f / 120);
        CHECK(world.body(fixed_first ? b : a).position.x < -0.95f);
    }
}

TEST_CASE("a segment crossing a circle reports its entry fraction") {
    const auto hit = segment_circle({0, -2}, {0, 2}, {}, 1);
    REQUIRE(hit);
    CHECK(near(*hit, 0.25f));
}

TEST_CASE("a stationary segment inside a circle hits at fraction zero") {
    const auto hit = segment_circle({}, {}, {}, 1);
    REQUIRE(hit);
    CHECK(near(*hit, 0));
}

TEST_CASE("segments outside a circle miss it") {
    CHECK_MESSAGE(!segment_circle({2, -2}, {2, 2}, {}, 1), "Circle sweep false positive");
    CHECK_MESSAGE(!segment_circle({2, 0}, {2, 0}, {}, 1), "Stationary outside segment hit");
}

TEST_CASE("a negative circle radius is rejected") {
    CHECK_THROWS_AS(segment_circle({}, {}, {}, -1), std::invalid_argument);
}

TEST_CASE("a grid segment stops at the first solid cell or the view boundary") {
    const auto grid = center_cell_grid();
    const auto straight = segment_grid({-2, 0}, {2, 0}, grid);
    REQUIRE(straight);
    CHECK(near(*straight, 0.25f));
    const auto diagonal = segment_grid({-2, -2}, {2, 2}, grid);
    REQUIRE(diagonal);
    CHECK(near(*diagonal, 0.25f));
    const auto outward = segment_grid({-2, -2}, {-4, -2}, grid);
    REQUIRE(outward);
    CHECK(near(*outward, 0.5f));
}

TEST_CASE("clear and stationary grid segments miss") {
    const auto grid = center_cell_grid();
    CHECK_MESSAGE(!segment_grid({-2, -2}, {2, -2}, grid), "Clear grid ray hit");
    CHECK_MESSAGE(!segment_grid({-2, -2}, {-2, -2}, grid), "Stationary empty grid ray hit");
}

TEST_CASE("an inconsistent collision grid is rejected") {
    const CollisionGrid empty{{}, 2, 2, 1, {}};
    CHECK_THROWS_AS(segment_grid({}, {}, empty), std::invalid_argument);
}

TEST_CASE("grid collision stops a controlled actor at a solid cell") {
    const auto actor = grid_actor();
    CHECK(actor.position.x < -1.49f);
    CHECK(actor.velocity.x == 0);
}

TEST_CASE("a contact transfers momentum from a controlled actor to a crate") {
    const auto push = pushed_crate();
    CHECK(push.crate.velocity.x > 0);
    CHECK(push.actor.velocity.x < 3);
}

TEST_CASE("a pushed crate keeps coasting") {
    auto crate = pushed_crate().crate;
    const auto old = crate.position;
    integrate(crate, 0.5f, 0.2f);
    CHECK(crate.position.x > old.x);
    CHECK(crate.velocity.x > 0);
}

TEST_CASE("a negative integration step is rejected") {
    auto crate = pushed_crate().crate;
    integrate(crate, 0.5f, 0.2f);
    CHECK_THROWS_AS(integrate(crate, -1), std::invalid_argument);
}

TEST_CASE("an off-center impulse adds torque") {
    Body spinning;
    apply_body_impulse(spinning, {2, 0}, {0, 1});
    CHECK(spinning.velocity.x == 2);
    CHECK(spinning.angular_velocity < 0);
}

TEST_CASE("a fixed body ignores impulses") {
    Body spinning;
    apply_body_impulse(spinning, {2, 0}, {0, 1});
    spinning.motion = Motion::fixed;
    const auto velocity = spinning.velocity;
    apply_body_impulse(spinning, {2, 0}, {});
    CHECK(near(spinning.velocity.x, velocity.x));
}
