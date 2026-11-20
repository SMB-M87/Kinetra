// Contract tests for moving-pair sweeps and swept world contacts (include/kinetra/simulation/physics.h):
// relative queries, fast pairs, crossing and overtaking, chronological chains, captured
// regression geometry and the observable swept-event budget.
#include <kinetra/simulation/physics.h>

#include <doctest/doctest.h>

#include <cmath>
#include <cstddef>

using namespace kinetra;

namespace {
/// Elastic, frictionless body: shape 0 is a circle, 1 a box and 2 a triangle.
Body agent(int shape, Vec2 position, Vec2 velocity) {
    Body body;
    if (shape == 2)
        body = regular_polygon(3, 0.2f);
    else
        body.shape = shape == 0 ? Shape::circle : Shape::box;
    body.radius = 0.2f;
    body.half_extents = {0.2f, 0.3f};
    body.position = position;
    body.velocity = velocity;
    body.restitution = 1;
    body.friction = 0;
    return body;
}

/// Resting circle at x = -2; the pair queries move it to x = 2.
Body left_circle() {
    return agent(0, {-2, 0}, {});
}

/// Resting circle at x = 2; the pair queries move it to x = -2.
Body right_circle() {
    return agent(0, {2, 0}, {});
}

/// Two fixed elastic walls at x = -1 and x = 1 with a circle fired between them at
/// 60000 units/s, after one 60 Hz step: far more bounces than the event budget allows.
World exhausted_corridor() {
    World world;
    Body wall;
    wall.shape = Shape::box;
    wall.motion = Motion::fixed;
    wall.half_extents = {0.05f, 3};
    wall.restitution = 1;
    wall.friction = 0;
    wall.position = {-1, 0};
    world.add(wall);
    wall.position = {1, 0};
    world.add(wall);
    world.add(agent(0, {}, {60000, 0}));
    world.step(1.f / 60);
    return world;
}
} // namespace

TEST_CASE("a head-on pair sweep reports the exact impact time and contact") {
    const auto hit = sweep_translation(left_circle(), {2, 0}, right_circle(), {-2, 0});
    REQUIRE(hit);
    CHECK(std::abs(hit->fraction - 0.45f) < 0.00001f);
    CHECK(length(hit->contact.point) < 0.0001f);
    CHECK(length(hit->contact.normal - Vec2{1, 0}) < 0.0001f);
}

TEST_CASE("a pair sweep maps the relative contact into world coordinates") {
    const auto a = left_circle(), b = right_circle();
    const auto hit = sweep_translation(a, {2, 0}, b, {-2, 0});
    const Vec2 drift{3, -4};
    const auto drifting = sweep_translation(a, Vec2{2, 0} + drift, b, Vec2{-2, 0} + drift);
    REQUIRE(hit);
    REQUIRE(drifting);
    CHECK(std::abs(drifting->fraction - hit->fraction) < 0.00001f);
    CHECK(length(drifting->contact.point - (hit->contact.point + drift * hit->fraction)) < 0.0001f);
}

TEST_CASE("a pair sweep in reversed argument order only flips the normal") {
    const auto a = left_circle(), b = right_circle();
    const auto hit = sweep_translation(a, {2, 0}, b, {-2, 0});
    const auto reversed = sweep_translation(b, {-2, 0}, a, {2, 0});
    REQUIRE(hit);
    REQUIRE(reversed);
    CHECK(std::abs(reversed->fraction - hit->fraction) < 0.00001f);
    CHECK(length(reversed->contact.normal + hit->contact.normal) < 0.0001f);
}

TEST_CASE("a co-moving separated pair does not collide") {
    CHECK_FALSE(sweep_translation(left_circle(), Vec2{2, 0}, right_circle(), Vec2{6, 0}));
}

TEST_CASE("fast controlled pairs of every shape stop at contact without exchanging sides") {
    for (int shape_a = 0; shape_a < 3; ++shape_a)
        for (int shape_b = 0; shape_b < 3; ++shape_b) {
            CAPTURE(shape_a);
            CAPTURE(shape_b);
            World world;
            auto a = agent(shape_a, {-2, 0}, {600, 0});
            auto b = agent(shape_b, {2, 0}, {-600, 0});
            a.motion = b.motion = Motion::controlled;
            world.add(a);
            world.add(b);
            world.step(1.f / 60);
            CHECK_MESSAGE(world.body(0).position.x < world.body(1).position.x,
                          "Fast controlled agents exchanged sides without contact");
            const auto contact = collide(world.body(0), world.body(1));
            CHECK_MESSAGE((!contact || contact->penetration < 0.001f),
                          "Controlled pair remained deeply overlapped");
            CHECK_MESSAGE(!world.sweep_budget_exhausted(), "Single pair exhausted swept-event budget");
        }
}

TEST_CASE("fast elastic pairs exchange velocities and keep their remaining travel") {
    for (int shape = 0; shape < 2; ++shape) {
        CAPTURE(shape);
        World world;
        world.add(agent(shape, {-2, 0}, {600, 0}));
        world.add(agent(shape, {2, 0}, {-600, 0}));
        world.step(1.f / 60);
        const auto &a = world.body(0), &b = world.body(1);
        CHECK_MESSAGE(std::abs(a.velocity.x + 600) < 0.001f,
                      "Elastic moving pair did not exchange velocities");
        CHECK_MESSAGE(std::abs(b.velocity.x - 600) < 0.001f,
                      "Elastic moving pair did not exchange velocities");
        CHECK_MESSAGE(std::abs(a.position.x + 8.4f) < 0.001f,
                      "Moving pair lost remaining travel after impact");
        CHECK_MESSAGE(std::abs(b.position.x - 8.4f) < 0.001f,
                      "Moving pair lost remaining travel after impact");
        CHECK_MESSAGE(length(a.velocity + b.velocity) < 0.001f, "Moving pair lost momentum");
        CHECK_MESSAGE(!world.sweep_budget_exhausted(), "Elastic pair exhausted swept-event budget");
    }
}

TEST_CASE("perpendicular controlled agents react instead of crossing") {
    World crossing;
    auto a = agent(0, {-2, 0}, {600, 0}), b = agent(0, {0, -2}, {0, 600});
    a.motion = b.motion = Motion::controlled;
    crossing.add(a);
    crossing.add(b);
    crossing.step(1.f / 60);
    CHECK(length(crossing.body(0).velocity - a.velocity) > 1);
    CHECK(length(crossing.body(0).position - crossing.body(1).position) > 0.399f);
    CHECK_MESSAGE(!crossing.sweep_budget_exhausted(), "Crossing pair exhausted swept-event budget");
}

TEST_CASE("an overtaking contact has the correct timing and remaining travel") {
    World overtaking;
    overtaking.add(agent(0, {-2, 0}, {600, 0}));
    overtaking.add(agent(0, {0, 0}, {300, 0}));
    overtaking.step(1.f / 60);
    CHECK(std::abs(overtaking.body(0).velocity.x - 300) < 0.001f);
    CHECK(std::abs(overtaking.body(1).velocity.x - 600) < 0.001f);
    CHECK(std::abs(overtaking.body(0).position.x - 4.6f) < 0.001f);
    CHECK(std::abs(overtaking.body(1).position.x - 8.4f) < 0.001f);
}

TEST_CASE("co-moving agents keep their separation") {
    World together;
    together.add(agent(0, {-2, 0}, {600, 0}));
    together.add(agent(0, {2, 0}, {600, 0}));
    together.step(1.f / 60);
    CHECK(std::abs(together.body(1).position.x - together.body(0).position.x - 4) < 0.001f);
}

TEST_CASE("a contact chain is processed in chronological order regardless of insertion order") {
    for (bool reverse : {false, true}) {
        CAPTURE(reverse);
        World world;
        std::size_t ids[3];
        for (int k = 0; k < 3; ++k) {
            const int i = reverse ? 2 - k : k;
            ids[i] = world.add(agent(0, {-2.f + static_cast<float>(2 * i), 0}, {i == 0 ? 600.f : 0.f, 0}));
        }
        world.step(1.f / 60);
        CHECK_MESSAGE(std::abs(world.body(ids[0]).position.x + 0.4f) < 0.001f,
                      "Contact chain was processed out of chronological order");
        CHECK_MESSAGE(std::abs(world.body(ids[1]).position.x - 1.6f) < 0.001f,
                      "Contact chain was processed out of chronological order");
        CHECK_MESSAGE(std::abs(world.body(ids[2]).position.x - 8.8f) < 0.001f,
                      "Contact chain was processed out of chronological order");
        CHECK_MESSAGE(std::abs(world.body(ids[2]).velocity.x - 600) < 0.001f,
                      "Contact chain lost final impulse");
        CHECK_MESSAGE(!world.sweep_budget_exhausted(), "Contact chain exhausted budget");
    }
}

TEST_CASE("a spinning tangent contact keeps making progress within the event budget") {
    // Captured from the crowded Demo, seed 123, tick 1383. Float contact geometry
    // gives a positive linear approach but a separating contact-point velocity.
    World world;
    Body circle;
    circle.motion = Motion::controlled;
    circle.position = {-13.05731583f, -58.96405029f};
    circle.velocity = {0.1216924191f, 0.1282006353f};
    circle.radius = 0.3199999928f;
    circle.angle = -0.7424513102f;
    circle.angular_velocity = 5;
    circle.friction = 0;
    world.add(circle);
    Body wall;
    wall.shape = Shape::box;
    wall.motion = Motion::fixed;
    wall.position = {-11.66991425f, -59.3385849f};
    wall.half_extents = {0.8739304543f, 0.7712159157f};
    wall.angle = 5.427427769f;
    world.add(wall);
    for (int tick = 0; tick < 120; ++tick) {
        world.step(1.f / 120);
        REQUIRE_MESSAGE(!world.sweep_budget_exhausted(),
                        "Tangent spinning contact exhausted the swept-event budget");
        REQUIRE_MESSAGE(world.candidate_pairs() < 20,
                        "Tangent contact repeatedly rebuilt collision candidates");
    }
    CHECK_MESSAGE(length(world.body(0).position - circle.position) > 0.1f,
                  "Tangent spinning circle stopped making forward progress");
}

TEST_CASE("nearly opposing resting contacts do not starve the swept timeline") {
    // Seed 123, tick 4280: a moving circle touches a sleeping triangular agent
    // and terrain. Nearly opposing normals converged through thousands of
    // zero-time events even though the remaining approach was below solver slop.
    World world;
    auto triangle = regular_polygon(3, 0.32f);
    triangle.motion = Motion::fixed;
    triangle.position = {-18.44842148f, -59.41837311f};
    triangle.angle = -1.797484756f;
    triangle.friction = 0;
    world.add(triangle);
    Body circle;
    circle.position = {-17.84042931f, -59.43058014f};
    circle.velocity = {-0.6518954039f, -1.860282302f};
    circle.radius = 0.32f;
    circle.angle = -1.724964738f;
    circle.angular_velocity = -1.08178556f;
    circle.friction = 0;
    world.add(circle);
    Body terrain;
    terrain.motion = Motion::fixed;
    terrain.position = {-16.73590088f, -59.84957886f};
    terrain.radius = 0.8613312244f;
    terrain.restitution = 0.8f;
    terrain.friction = 0;
    world.add(terrain);
    for (int tick = 0; tick < 120; ++tick) {
        world.step(1.f / 120);
        REQUIRE_MESSAGE(!world.sweep_budget_exhausted(), "Resting gap contacts starved the swept timeline");
        REQUIRE_MESSAGE(world.candidate_pairs() < 64, "Resting gap repeatedly rebuilt collision candidates");
    }
}

TEST_CASE("the resting-contact tolerance does not swallow a tiny mover") {
    World world;
    Body wall;
    wall.shape = Shape::box;
    wall.motion = Motion::fixed;
    wall.half_extents = {0.0001f, 1};
    world.add(wall);
    Body circle;
    circle.motion = Motion::controlled;
    circle.radius = 0.0001f;
    circle.position = {-0.00019f, 0};
    circle.velocity = {0.1f, 0};
    world.add(circle);
    world.step(0.01f);
    CHECK(world.body(1).position.x < 0);
    CHECK(std::abs(world.body(1).velocity.x) < 0.000001f);
    CHECK_MESSAGE(!world.sweep_budget_exhausted(), "Tiny touching contact exhausted swept-event budget");
}

TEST_CASE("an exhausted event budget is reported and its remaining travel discarded") {
    const auto world = exhausted_corridor();
    CHECK(world.sweep_budget_exhausted());
    CHECK(std::abs(world.body(2).position.x) <= 0.751f);
}

TEST_CASE("the budget exhaustion flag resets on the next step") {
    auto world = exhausted_corridor();
    world.body(2).velocity = {};
    world.step(1.f / 60);
    CHECK_FALSE(world.sweep_budget_exhausted());
}

TEST_CASE("clearing the world resets the budget exhaustion flag") {
    auto world = exhausted_corridor();
    world.body(2).velocity = {};
    world.step(1.f / 60);
    world.body(2).velocity = {60000, 0};
    world.step(1.f / 60);
    world.clear();
    CHECK_FALSE(world.sweep_budget_exhausted());
}
