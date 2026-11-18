// Contract tests for convex polygon bodies (include/kinetra/physics.h): validation, bounds,
// exact and rigid-transform-invariant contacts, inertia, and their use by navigation,
// steering and a settling world.
#include <kinetra/navigation.h>
#include <kinetra/steering.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

using namespace kinetra;

namespace {
/// Unit regular polygon with `sides` sides, rotated by 0.37 and placed at (2, -3).
Body placed_polygon(int sides) {
    auto polygon = regular_polygon(sides, 1);
    polygon.angle = 0.37f;
    polygon.position = {2, -3};
    return polygon;
}

/// Circle of radius 0.1 at `position`.
Body small_circle(Vec2 position) {
    Body circle;
    circle.position = position;
    circle.radius = 0.1f;
    return circle;
}

/// A fixed unit triangle at the origin as the only obstacle.
std::vector<Body> triangle_obstacle() {
    auto triangle = regular_polygon(3, 1);
    triangle.motion = Motion::fixed;
    return {triangle};
}
} // namespace

TEST_CASE("polygon bounds contain every transformed vertex") {
    for (int sides : {3, 5, 6, 32}) {
        CAPTURE(sides);
        const auto polygon = placed_polygon(sides);
        const auto box = bounds(polygon);
        for (auto vertex : polygon.vertices) {
            const auto p = polygon.position + rotated(vertex, polygon.angle);
            CHECK(p.x >= box.min.x);
            CHECK(p.x <= box.max.x);
            CHECK(p.y >= box.min.y);
            CHECK(p.y <= box.max.y);
        }
    }
}

TEST_CASE("a circle contained in a polygon gets a finite exit contact") {
    for (int sides : {3, 5, 6, 32}) {
        CAPTURE(sides);
        auto polygon = placed_polygon(sides);
        auto circle = small_circle(polygon.position);
        const auto contact = collide(circle, polygon);
        REQUIRE(contact);
        CHECK(contact->penetration > 0);
        CHECK(std::abs(length(contact->normal) - 1) < 0.0001f);
        resolve_contact(circle, polygon, *contact);
        CHECK_MESSAGE(finite(circle.position), "Containment correction is not finite");
    }
}

TEST_CASE("a triangle collides by its shape and not by its bounding circle") {
    CHECK_FALSE(collide(regular_polygon(3, 1), small_circle({-0.8f, 0})));
}

TEST_CASE("triangle vertex contacts are symmetric") {
    const auto triangle = regular_polygon(3, 1);
    const auto circle = small_circle({1.05f, 0});
    const auto contact = collide(triangle, circle), reversed = collide(circle, triangle);
    REQUIRE(contact);
    REQUIRE(reversed);
    CHECK(length(contact->normal + reversed->normal) < 0.0001f);
}

TEST_CASE("polygon pair contacts are found and rigid-transform invariant") {
    const auto triangle = regular_polygon(3, 1);
    auto hexagon = regular_polygon(6, 1);
    hexagon.position = {1.4f, 0.15f};
    hexagon.angle = 0.2f;
    Body box_body;
    box_body.shape = Shape::box;
    for (const auto &other : {hexagon, box_body}) {
        INFO("other shape ", static_cast<int>(other.shape));
        const auto hit = collide(triangle, other);
        REQUIRE_MESSAGE(hit, "Polygon pair contact missing");
        CHECK_MESSAGE(hit->penetration > 0, "Polygon pair contact missing");
        const float angle = 0.73f;
        auto a = triangle, b = other;
        a.angle += angle;
        b.angle += angle;
        a.position = rotated(a.position, angle) + Vec2{4, -2};
        b.position = rotated(b.position, angle) + Vec2{4, -2};
        const auto transformed = collide(a, b);
        REQUIRE(transformed);
        CHECK(std::abs(transformed->penetration - hit->penetration) < 0.0001f);
        CHECK(length(transformed->normal - rotated(hit->normal, angle)) < 0.0001f);
        CHECK(length(transformed->point - (rotated(hit->point, angle) + Vec2{4, -2})) < 0.0001f);
    }
}

TEST_CASE("polygon inertia gives the expected spin from an off-center impulse") {
    auto square = regular_polygon(4, 1);
    apply_body_impulse(square, {0, 1}, {1, 0});
    CHECK(std::abs(square.angular_velocity - 3) < 0.0001f);
}

TEST_CASE("a polygon not centered at its centroid is rejected") {
    auto square = regular_polygon(4, 1);
    square.vertices.pop_back();
    CHECK_THROWS_AS_MESSAGE(validate(square), std::invalid_argument, "square with a removed vertex");
    square = regular_polygon(4, 1);
    for (auto &v : square.vertices)
        v.x += 1;
    CHECK_THROWS_AS_MESSAGE(validate(square), std::invalid_argument, "square shifted off its origin");
}

TEST_CASE("a clockwise polygon is rejected") {
    auto square = regular_polygon(4, 1);
    std::reverse(square.vertices.begin(), square.vertices.end());
    CHECK_THROWS_AS(validate(square), std::invalid_argument);
}

TEST_CASE("a polygon with a repeated vertex is rejected") {
    auto square = regular_polygon(4, 1);
    square.vertices[1] = square.vertices[0];
    CHECK_THROWS_AS(validate(square), std::invalid_argument);
}

TEST_CASE("a concave polygon is rejected") {
    auto pentagon = regular_polygon(5, 1);
    pentagon.vertices[1] = {};
    CHECK_THROWS_AS(validate(pentagon), std::invalid_argument);
}

TEST_CASE("navigation blocks a polygon obstacle but not its bypass") {
    Navigation navigation(0.25f, 0.1f);
    navigation.rebuild({{-4, -4}, {4, 4}}, triangle_obstacle());
    CHECK_MESSAGE(!navigation.clear({-2, 0}, {2, 0}), "Navigation crossed polygon obstacle");
    CHECK_MESSAGE(navigation.clear({-2, 2}, {2, 2}), "Navigation blocked a clear polygon bypass");
}

TEST_CASE("steering around a polygon obstacle stays finite") {
    const auto obstacles = triangle_obstacle();
    CHECK(finite(steer(regular_polygon(3, 0.3f), {3, 0}, obstacles)));
}

TEST_CASE("a falling polygon settles on a floor") {
    World world;
    Body floor;
    floor.shape = Shape::box;
    floor.motion = Motion::fixed;
    floor.position = {0, -1};
    floor.half_extents = {5, 0.5f};
    world.add(floor);
    auto falling = regular_polygon(6, 0.5f);
    falling.position = {0, 2};
    falling.restitution = 0;
    world.add(falling);
    world.gravity = {0, -9.81f};
    for (int i = 0; i < 600; ++i)
        world.step(1.f / 120);
    CHECK(finite(world.body(1).position));
    CHECK(bounds(world.body(1)).min.y > -0.52f);
    CHECK(length(world.body(1).velocity) < 0.1f);
}
