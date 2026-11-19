// Contract tests for kinetra::Navigation (include/kinetra/simulation/navigation.h) and the public math
// boundaries it relies on: invalid settings, swept clearance, exact and projected goals.
#include <kinetra/simulation/navigation.h>

#include <doctest/doctest.h>

#include <limits>
#include <stdexcept>

using namespace kinetra;

namespace {
/// Navigation with resolution 0.5 and radius 0.2 over [-5, 5]^2, around a 1 by 4 box at the origin.
Navigation around_box() {
    Navigation navigation(0.5f, 0.2f);
    Body obstacle;
    obstacle.shape = Shape::box;
    obstacle.position = {0, 0};
    obstacle.half_extents = {0.5f, 2};
    navigation.rebuild({{-5, -5}, {5, 5}}, {obstacle});
    return navigation;
}
} // namespace

TEST_CASE("invalid navigation settings are rejected") {
    CHECK_THROWS_AS(Navigation(0, 1), std::invalid_argument);
    CHECK_THROWS_AS(Navigation(1, -1), std::invalid_argument);
    CHECK_THROWS_AS(Navigation(std::numeric_limits<float>::infinity(), 1), std::invalid_argument);
}

TEST_CASE("a navigation sweep does not cross an obstacle") {
    CHECK_FALSE(around_box().clear({-2, 0}, {2, 0}));
}

TEST_CASE("a free goal keeps its exact position at the end of the path") {
    const auto path = around_box().path({-2, 0}, {2.123f, 0.123f});
    REQUIRE_FALSE(path.empty());
    CHECK(distance(path.back(), {2.123f, 0.123f}) < 0.00001f);
}

TEST_CASE("every segment of a planned path is clear") {
    const auto navigation = around_box();
    const auto path = navigation.path({-2, 0}, {2.123f, 0.123f});
    Vec2 previous{-2, 0};
    for (auto point : path) {
        CHECK(navigation.clear(previous, point));
        previous = point;
    }
}

TEST_CASE("an occupied goal is projected to a clear endpoint") {
    const auto navigation = around_box();
    const auto projected = navigation.path({-2, 0}, {});
    REQUIRE_FALSE(projected.empty());
    CHECK(navigation.clear(projected.back(), projected.back()));
}

TEST_CASE("a NaN goal yields no path") {
    CHECK(around_box().path({-2, 0}, {std::numeric_limits<float>::quiet_NaN(), 0}).empty());
}

TEST_CASE("the distance to a degenerate segment is the distance to its point") {
    CHECK(distance_to_segment({1, 0}, {}, {}) == 1);
}

TEST_CASE("normalizing a zero vector gives a zero vector") {
    CHECK(length(normalized({})) == 0);
}

TEST_CASE("an infinite vector is not finite") {
    CHECK_FALSE(finite({std::numeric_limits<float>::infinity(), 0}));
}

TEST_CASE("lerp interpolates halfway between its endpoints") {
    CHECK(distance(lerp({}, {2, 4}, 0.5f), {1, 2}) == 0);
}

TEST_CASE("touching AABBs overlap") {
    CHECK(Aabb{{0, 0}, {1, 1}}.overlaps({{1, 1}, {2, 2}}));
}
