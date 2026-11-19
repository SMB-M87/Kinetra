// Contract tests for circle sweeps (include/kinetra/simulation/physics.h): impact timing against walls,
// circles and polygons, corners, initial overlap, rigid transforms, thin-wall world impacts,
// remaining travel and the bounded repeated-bounce budget.
#include <kinetra/simulation/physics.h>

#include <doctest/doctest.h>

#include <cmath>
#include <stdexcept>

using namespace kinetra;

namespace {
/// Fixed, elastic, frictionless wall at the origin, 0.1 wide and 4 high.
Body wall() {
    Body body;
    body.shape = Shape::box;
    body.motion = Motion::fixed;
    body.half_extents = {0.05f, 2};
    body.restitution = 1;
    body.friction = 0;
    return body;
}

/// Circle of radius 0.1 at x = -2, the mover of the query cases.
Body probe() {
    Body circle;
    circle.radius = 0.1f;
    circle.position = {-2, 0};
    return circle;
}

/// A circle of radius 0.1 fired at wall() from x = -2 at 600 units/s, after one step of `dt`.
World thin_wall_impact(Motion motion, float dt) {
    World world;
    world.add(wall());
    Body ball;
    ball.motion = motion;
    ball.radius = 0.1f;
    ball.position = {-2, 0};
    ball.velocity = {600, 0};
    ball.restitution = 1;
    ball.friction = 0;
    world.add(ball);
    world.step(dt);
    return world;
}

/// Walls at x = -1 and x = 1 with an elastic circle at the origin moving right at
/// 600 units/s, not yet stepped.
World corridor() {
    World world;
    auto left = wall(), right = wall();
    left.position = {-1, 0};
    right.position = {1, 0};
    world.add(left);
    world.add(right);
    Body ball;
    ball.radius = 0.1f;
    ball.velocity = {600, 0};
    ball.restitution = 1;
    ball.friction = 0;
    world.add(ball);
    return world;
}
} // namespace

TEST_CASE("a circle sweep hits a thin wall at the exact time and contact") {
    const auto hit = sweep_circle(probe(), {2, 0}, wall());
    REQUIRE(hit);
    CHECK(std::abs(hit->fraction - 0.4625f) < 0.00001f);
    CHECK(length(hit->contact.normal - Vec2{1, 0}) < 0.00001f);
    CHECK(std::abs(hit->contact.point.x + 0.05f) < 0.00001f);
}

TEST_CASE("circle sweeps are rigid-transform invariant") {
    const auto circle = probe();
    const auto obstacle = wall();
    const auto hit = sweep_circle(circle, {2, 0}, obstacle);
    const float angle = 0.63f;
    const Vec2 offset{3, -4};
    auto rotated_circle = circle, rotated_wall = obstacle;
    rotated_circle.position = rotated(circle.position, angle) + offset;
    rotated_wall.position = offset;
    rotated_wall.angle = angle;
    const auto transformed = sweep_circle(rotated_circle, rotated(Vec2{2, 0}, angle) + offset, rotated_wall);
    REQUIRE(hit);
    REQUIRE(transformed);
    CHECK(std::abs(transformed->fraction - hit->fraction) < 0.00001f);
    CHECK(length(transformed->contact.normal - rotated(hit->contact.normal, angle)) < 0.00001f);
    CHECK(length(transformed->contact.point - (rotated(hit->contact.point, angle) + offset)) < 0.00001f);
}

TEST_CASE("separating and stationary circle sweeps miss the wall") {
    const auto circle = probe();
    CHECK_MESSAGE(!sweep_circle(circle, {-3, 0}, wall()), "Separating sweep hit a wall");
    CHECK_MESSAGE(!sweep_circle(circle, circle.position, wall()), "Stationary separated sweep hit a wall");
}

TEST_CASE("an initial circle overlap is reported at time zero with penetration") {
    auto circle = probe();
    circle.position = {};
    const auto inside = sweep_circle(circle, {2, 0}, wall());
    REQUIRE(inside);
    CHECK(inside->fraction == 0);
    CHECK(inside->contact.penetration > 0);
}

TEST_CASE("a grazing corner sweep hits but a near-corner miss does not") {
    auto circle = probe();
    circle.position = {-2, 2.1f};
    CHECK_MESSAGE(sweep_circle(circle, {2, 2.1f}, wall()).has_value(), "Grazing corner sweep missed");
    circle.position.y = 2.11f;
    CHECK_MESSAGE(!sweep_circle(circle, {2, 2.11f}, wall()), "Near-corner miss became a false hit");
}

TEST_CASE("a circle sweep against a circle hits at the exact time") {
    Body target;
    target.radius = 0.5f;
    const auto round = sweep_circle(probe(), {2, 0}, target);
    REQUIRE(round);
    CHECK(std::abs(round->fraction - 0.35f) < 0.00001f);
}

TEST_CASE("a circle sweep against a triangle face uses the polygon envelope") {
    const auto polygon = sweep_circle(probe(), {2, 0}, regular_polygon(3, 1));
    REQUIRE(polygon);
    CHECK(std::abs(polygon->fraction - 0.35f) < 0.00001f);
}

TEST_CASE("a circle sweep against a triangle vertex hits the rounded corner") {
    auto circle = probe();
    circle.position = {2, 0};
    const auto vertex = sweep_circle(circle, {-2, 0}, regular_polygon(3, 1));
    REQUIRE(vertex);
    CHECK(std::abs(vertex->fraction - 0.225f) < 0.00001f);
}

TEST_CASE("a noncircle sweep mover is rejected") {
    CHECK_THROWS_AS(sweep_circle(regular_polygon(3, 1), {}, wall()), std::invalid_argument);
}

TEST_CASE("a fast dynamic circle bounces off a thin wall and keeps its remaining travel") {
    for (const float dt : {1.f / 60, 1.f / 120}) {
        CAPTURE(dt);
        const auto world = thin_wall_impact(Motion::dynamic, dt);
        CHECK_MESSAGE(world.body(1).position.x <= -0.1499f, "Fast circle tunneled through a fixed wall");
        CHECK_MESSAGE(world.body(0).position.x == 0, "Swept impact moved fixed obstacle");
        CHECK_MESSAGE(std::abs(world.body(1).velocity.x + 600) < 0.001f, "Elastic swept bounce lost speed");
        CHECK_MESSAGE(std::abs(world.body(1).position.x - (-2 + 3.7f - 600 * dt)) < 0.001f,
                      "Swept bounce did not consume remaining travel");
    }
}

TEST_CASE("a fast controlled circle stops at a thin wall without bouncing") {
    for (const float dt : {1.f / 60, 1.f / 120}) {
        CAPTURE(dt);
        const auto world = thin_wall_impact(Motion::controlled, dt);
        CHECK_MESSAGE(world.body(1).position.x <= -0.1499f, "Fast circle tunneled through a fixed wall");
        CHECK_MESSAGE(world.body(0).position.x == 0, "Swept impact moved fixed obstacle");
        CHECK_MESSAGE(length(world.body(1).velocity) < 0.001f, "Controlled circle bounced");
    }
}

TEST_CASE("world sweeps stop at rotated fixed geometry") {
    for (int shape = 0; shape < 3; ++shape) {
        CAPTURE(shape);
        World world;
        Body obstacle = shape == 2 ? regular_polygon(3, 1) : wall();
        if (shape == 1) {
            obstacle.shape = Shape::circle;
            obstacle.radius = 0.5f;
        }
        obstacle.motion = Motion::fixed;
        obstacle.angle = 0.63f;
        world.add(obstacle);
        Body ball;
        ball.radius = 0.1f;
        ball.motion = Motion::controlled;
        ball.position = rotated({-2, 0}, obstacle.angle);
        ball.velocity = rotated({600, 0}, obstacle.angle);
        world.add(ball);
        world.step(1.f / 60);
        const auto local = rotated(world.body(1).position, -obstacle.angle);
        const float limit = shape == 0 ? -0.15f : -0.6f;
        CHECK(local.x <= limit + 0.0001f);
        CHECK(length(world.body(1).velocity) < 0.001f);
    }
}

TEST_CASE("repeated circle impacts stay inside a corridor without losing speed") {
    auto world = corridor();
    world.step(1.f / 60);
    CHECK(std::abs(world.body(2).position.x) <= 0.851f);
    CHECK(std::abs(std::abs(world.body(2).velocity.x) - 600) < 0.001f);
}

TEST_CASE("an exhausted impact budget still keeps the body inside the corridor") {
    // Very many bounces exhaust the bounded impact budget: keep the body inside.
    auto world = corridor();
    world.step(1.f / 60);
    world.body(2).velocity = {60000, 0};
    world.step(1.f / 60);
    CHECK(std::abs(world.body(2).position.x) <= 0.851f);
}
