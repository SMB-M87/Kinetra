// Contract tests for convex translation sweeps (include/kinetra/simulation/physics.h): every convex
// mover against walls, circles and polygons, timing, orientation, misses, thin-wall world
// impacts, repeated impacts and the free rotational integration sweeps must not change.
#include <kinetra/simulation/physics.h>

#include <doctest/doctest.h>

#include <cmath>
#include <limits>
#include <stdexcept>

using namespace kinetra;

namespace {
/// Number of mover() variants.
constexpr int mover_variants = 4;

/// Fixed, elastic, frictionless wall at the origin, 0.1 wide and 6 high.
Body wall() {
    Body body;
    body.shape = Shape::box;
    body.half_extents = {0.05f, 3};
    body.motion = Motion::fixed;
    body.restitution = 1;
    body.friction = 0;
    return body;
}

/// Elastic, frictionless mover at x = -2: variant 0 is a circle, 1 a box, 2 a triangle
/// and 3 a hexagon.
Body mover(int variant) {
    Body body;
    if (variant >= 2)
        body = regular_polygon(variant == 2 ? 3 : 6, 0.2f);
    else
        body.shape = variant == 0 ? Shape::circle : Shape::box;
    body.radius = 0.2f;
    body.half_extents = {0.2f, 0.3f};
    body.position = {-2, 0};
    body.restitution = 1;
    body.friction = 0;
    return body;
}

/// A wall of 0.4 by 0.4 at the origin, for the miss cases.
Body small_wall() {
    auto small = wall();
    small.half_extents = {0.2f, 0.2f};
    return small;
}
} // namespace

TEST_CASE("every convex mover hits a thin wall at the exact time and contact") {
    for (int variant = 0; variant < mover_variants; ++variant) {
        CAPTURE(variant);
        const auto hit = sweep_translation(mover(variant), {2, 0}, wall());
        REQUIRE(hit);
        CHECK(std::abs(hit->fraction - 0.4375f) < 0.00001f);
        CHECK(length(hit->contact.normal - Vec2{1, 0}) < 0.0001f);
        CHECK(std::abs(hit->contact.point.x + 0.05f) < 0.0001f);
    }
}

TEST_CASE("separating and stationary convex sweeps miss the wall") {
    for (int variant = 0; variant < mover_variants; ++variant) {
        CAPTURE(variant);
        const auto body = mover(variant);
        CHECK_MESSAGE(!sweep_translation(body, {-3, 0}, wall()), "Separating convex sweep hit wall");
        CHECK_MESSAGE(!sweep_translation(body, body.position, wall()),
                      "Separated stationary convex sweep hit wall");
    }
}

TEST_CASE("an initial convex overlap is reported at time zero") {
    for (int variant = 0; variant < mover_variants; ++variant) {
        CAPTURE(variant);
        auto touching = mover(variant);
        touching.position.x = -0.2f;
        const auto initial = sweep_translation(touching, {2, 0}, wall());
        REQUIRE(initial);
        CHECK(initial->fraction == 0);
    }
}

TEST_CASE("convex sweeps are rigid-transform invariant") {
    for (int variant = 0; variant < mover_variants; ++variant) {
        CAPTURE(variant);
        const auto body = mover(variant);
        const auto obstacle = wall();
        const auto hit = sweep_translation(body, {2, 0}, obstacle);
        const float angle = 0.63f;
        const Vec2 offset{3, -4};
        auto rotated_body = body, rotated_wall = obstacle;
        rotated_body.position = rotated(body.position, angle) + offset;
        rotated_body.angle = rotated_wall.angle = angle;
        rotated_wall.position = offset;
        const auto transformed =
            sweep_translation(rotated_body, rotated(Vec2{2, 0}, angle) + offset, rotated_wall);
        REQUIRE(hit);
        REQUIRE(transformed);
        CHECK(std::abs(transformed->fraction - hit->fraction) < 0.00001f);
        CHECK(length(transformed->contact.normal - rotated(hit->contact.normal, angle)) < 0.0001f);
        CHECK(length(transformed->contact.point - (rotated(hit->contact.point, angle) + offset)) < 0.0001f);
    }
}

TEST_CASE("convex sweeps hit circles and polygons at the exact time") {
    for (int variant = 0; variant < mover_variants; ++variant) {
        CAPTURE(variant);
        const auto body = mover(variant);
        for (const bool round : {false, true}) {
            CAPTURE(round);
            auto target = round ? Body{} : regular_polygon(3, 1);
            target.radius = 0.5f;
            const auto result = sweep_translation(body, {2, 0}, target);
            REQUIRE(result);
            CHECK(std::abs(result->fraction - 0.325f) < 0.0001f);
        }
    }
}

TEST_CASE("a sweep accounts for the mover orientation") {
    auto box = mover(1);
    box.angle = 0.4f;
    const auto hit = sweep_translation(box, {2, 0}, wall());
    const float extent = std::cos(box.angle) * 0.2f + std::sin(box.angle) * 0.3f;
    REQUIRE(hit);
    CHECK(std::abs(hit->fraction - (2 - 0.05f - extent) / 4) < 0.00001f);
}

TEST_CASE("a convex near miss is not a false hit") {
    auto box = mover(1);
    box.position = {-2, 0.6f};
    CHECK_FALSE(sweep_translation(box, {2, 0.6f}, small_wall()));
}

TEST_CASE("disjoint sweep intervals do not become contact") {
    auto box = mover(1);
    box.position = {-2, -2};
    auto small = small_wall();
    small.position = {0, 1.5f};
    CHECK_FALSE(sweep_translation(box, {2, 2}, small));
}

TEST_CASE("a nonfinite translation endpoint is rejected") {
    auto box = mover(1);
    box.position = {-2, -2};
    auto small = small_wall();
    small.position = {0, 1.5f};
    CHECK_THROWS_AS(sweep_translation(box, {std::numeric_limits<float>::infinity(), 0}, small),
                    std::invalid_argument);
}

TEST_CASE("fast convex movers stop at a thin wall without tunneling") {
    for (int variant = 0; variant < mover_variants; ++variant)
        for (auto motion : {Motion::controlled, Motion::dynamic}) {
            CAPTURE(variant);
            INFO("controlled ", motion == Motion::controlled);
            World world;
            world.add(wall());
            auto body = mover(variant);
            body.motion = motion;
            body.velocity = {600, 0};
            world.add(body);
            world.step(1.f / 60);
            CHECK_MESSAGE(world.body(1).position.x <= -0.2499f, "Convex mover tunneled through thin wall");
            CHECK_MESSAGE(world.body(1).velocity.x <= 0,
                          "Convex swept impact did not remove approach velocity");
            if (motion == Motion::controlled)
                CHECK_MESSAGE(length(world.body(1).velocity) < 0.001f, "Controlled convex body bounced");
        }
}

TEST_CASE("repeated convex impacts stay inside a corridor without losing speed") {
    World world;
    auto left = wall(), right = wall();
    left.position = {-1, 0};
    right.position = {1, 0};
    world.add(left);
    world.add(right);
    auto body = mover(1);
    body.position = {};
    body.velocity = {600, 0};
    world.add(body);
    world.step(1.f / 60);
    CHECK(std::abs(world.body(2).position.x) <= 0.751f);
    CHECK(std::abs(std::abs(world.body(2).velocity.x) - 600) < 0.001f);
}

TEST_CASE("a rotating body without obstacles keeps free integration") {
    World free;
    auto body = mover(1);
    body.velocity = {6, 2};
    body.angular_velocity = 4;
    free.add(body);
    free.step(1.f / 60);
    CHECK(length(free.body(0).position - (body.position + body.velocity / 60)) < 0.0001f);
    CHECK(std::abs(free.body(0).angle - 4.f / 60) < 0.0001f);
}
