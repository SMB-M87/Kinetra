// Contract tests for rectangular navigation, priority steering and predictive passing.
#include <kinetra/simulation/passing.h>
#include <kinetra/simulation/steering.h>
#include <kinetra/simulation/weighted_grid.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
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

/// Priority steering tuning for these test footprints and steps.
PrioritySteering steering_settings(float arrival_radius = 10) {
    return {.max_speed = 2,
            .max_acceleration = 0.6f,
            .arrival_radius = arrival_radius,
            .border_distance = 2,
            .border_strength = 10,
            .border_lookahead = 2,
            .prediction_steps = 8,
            .prediction_step = 1,
            .safety_multiplier = 1.025f};
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
/// priority_steer's predictive term computed as before it searched an interval of steps:
/// every pair of steps (i, j) in order, the first overlapping one per neighbor.
Vec2 brute_force_predictive(const Body &a, std::span<const Body> neighbors, const PrioritySteering &s) {
    Vec2 sum{};
    int predicted = 0;
    for (const auto &other : neighbors) {
        if (length(a.velocity) == 0 ||
            distance(a.position, other.position) >
                length(a.half_extents) + length(other.half_extents) +
                    static_cast<float>(2 * s.prediction_steps) * s.prediction_step * s.max_speed)
            continue;
        bool found = false;
        for (int i = 0; i <= s.prediction_steps && !found; ++i)
            for (int j = 0; j <= s.prediction_steps; ++j) {
                const Vec2 diff =
                    (a.position + a.velocity * (static_cast<float>(i) * s.prediction_step)) -
                    (other.position + other.velocity * (static_cast<float>(j) * s.prediction_step));
                const Vec2 extent = (a.half_extents + other.half_extents) * s.safety_multiplier;
                if (std::abs(diff.x) < extent.x && std::abs(diff.y) < extent.y) {
                    sum += normalized(diff);
                    ++predicted;
                    found = true;
                    break;
                }
            }
    }
    return predicted ? sum / float(predicted) : sum;
}
void check_passing_point(Vec2 actual, Vec2 expected) {
    CHECK(actual.x == expected.x);
    CHECK(actual.y == expected.y);
}
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

TEST_CASE("a search ends at the center of the accepted cell nearest by route cost") {
    // From cell 0, cell 10 (two rows down) costs 20 and cell 4 (four columns across) 40.
    const auto grid = corridor_grid();
    const auto route = grid.search({5, 5}, [](int cell) { return cell == 4 || cell == 10; });
    REQUIRE_FALSE(route.empty());
    CHECK(grid.cell(route.back()) == 10);
    CHECK(distance(route.back(), grid.center(10)) == 0);
    for (std::size_t i = 0; i + 1 < route.size(); ++i)
        CHECK(distance(route[i], grid.center(grid.cell(route[i]))) == 0);
}

TEST_CASE("a search offers every reachable cell once in nondecreasing route cost, the start first") {
    const auto grid = corridor_grid();
    std::vector<int> offered;
    CHECK(grid.search({5, 5},
                      [&](int cell) {
                          offered.push_back(cell);
                          return false;
                      })
              .empty());
    REQUIRE(offered.size() == 15u);
    CHECK(offered.front() == 0);
    // An open grid: the route cost of a cell is its octile distance from cell 0, in cells.
    const auto cost = [](int cell) {
        const int x = cell % 5, y = cell / 5;
        return 10.f * (float(std::max(x, y)) + (std::sqrt(2.f) - 1) * float(std::min(x, y)));
    };
    for (std::size_t i = 0; i + 1 < offered.size(); ++i)
        CHECK(cost(offered[i]) <= cost(offered[i + 1]) + 1e-4f);
    std::vector<int> sorted = offered;
    std::sort(sorted.begin(), sorted.end());
    CHECK(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end());
}

TEST_CASE("a search counts accepted cells in route order, as a counting predicate expects") {
    // Accept the third cell of the bottom row that the search reaches: cell 12.
    const auto grid = corridor_grid();
    int seen = 0;
    const auto route = grid.search({5, 25}, [&](int cell) { return cell >= 10 && ++seen == 3; });
    REQUIRE_FALSE(route.empty());
    CHECK(grid.cell(route.back()) == 12);
}

TEST_CASE("a search enters only admitted cells and fails when none leads to an accepted one") {
    const auto grid = corridor_grid();
    // Column 2 is closed to the route, so cell 4 cannot be reached from cell 0.
    const auto outside_column_2 = [](int cell) { return cell % 5 != 2; };
    CHECK(grid.search({5, 5}, [](int cell) { return cell == 4; }, outside_column_2).empty());
    const auto route = grid.search({5, 5}, [](int cell) { return cell == 11; }, outside_column_2);
    REQUIRE_FALSE(route.empty());
    for (const auto point : route)
        CHECK(grid.cell(point) % 5 != 2);
}

TEST_CASE("a search follows weights and blocked corners like path") {
    const auto blocked = blocked_corridor();
    CHECK(blocked.search({5, 5}, [](int cell) { return cell == 6; }).empty());
    auto grid = corridor_grid();
    grid.weights(congestion());
    for (const auto point : grid.search({5, 15}, [](int cell) { return cell == 9; }))
        CHECK(grid.cell(point) != 7);
}

TEST_CASE("a search whose start cell is accepted goes to that cell's center") {
    const auto grid = corridor_grid();
    const auto route = grid.search({2, 3}, [](int) { return true; });
    REQUIRE(route.size() == 1u);
    CHECK(distance(route.front(), {5, 5}) == 0);
}

TEST_CASE("a search from a blocked or outside start is empty and negative extents throw") {
    const auto grid = blocked_corridor();
    CHECK(grid.search({15, 5}, [](int) { return true; }).empty());
    CHECK(grid.search({-1, 5}, [](int) { return true; }).empty());
    CHECK_THROWS_AS(grid.search({5, 5}, [](int) { return true; }, {}, {-1, 0}), std::invalid_argument);
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

TEST_CASE("a raster route target consumes visible waypoints and keeps the exact goal") {
    const auto grid = corridor_grid();
    const Vec2 position{5, 15};
    const Vec2 goal{44, 16};
    auto route = grid.path(position, goal);
    REQUIRE(route.size() > 1);

    CHECK(distance(route_target(position, route, grid, {}), goal) == 0);
    REQUIRE(route.size() == 1);
    CHECK(distance(route.front(), goal) == 0);
    CHECK(distance(route_target(position, route, grid, {}), goal) == 0);

    route.clear();
    CHECK(distance(route_target(position, route, grid, {}), position) == 0);
    CHECK(route.empty());
}

TEST_CASE("raster corner lookahead requires a clear sweep") {
    const auto grid = footprint_grid();
    const Vec2 position{15, 25};
    const Vec2 corner{15, 15};
    const Vec2 goal{45, 15};
    std::vector<Vec2> route{corner, goal};
    REQUIRE_FALSE(grid.clear(position, goal));

    CHECK(distance(route_target(position, route, grid, {}), corner) == 0);
    CHECK(distance(route_target(position, route, grid, {.lookahead = 15}), {20, 15}) == 0);
    REQUIRE_FALSE(grid.clear(position, {37, 15}));
    CHECK(distance(route_target(position, route, grid, {.lookahead = 32}), corner) == 0);
    REQUIRE(route.size() == 2);
    CHECK(distance(route.front(), corner) == 0);

    CHECK(distance(route_target(corner, route, grid, {}), goal) == 0);
    REQUIRE(route.size() == 1);
}

TEST_CASE("a raster route target stops at the first obstruction after a straight run") {
    const auto grid = footprint_grid();
    const Vec2 position{15, 5};
    const Vec2 before_turn{35, 5};
    const Vec2 after_turn{35, 35};
    const Vec2 goal{5, 35};
    std::vector<Vec2> route{{20, 5}, {25, 5}, before_turn, after_turn, goal};
    REQUIRE(grid.clear(position, goal));
    REQUIRE_FALSE(grid.clear(position, after_turn));

    CHECK(distance(route_target(position, route, grid, {}), before_turn) == 0);
    REQUIRE(route.size() == 3);
    CHECK(distance(route[1], after_turn) == 0);
}

TEST_CASE("a straight raster shortcut retains the intermediate endpoint check") {
    const auto grid = footprint_grid();
    const Vec2 position{5, 35};
    const Vec2 first{10, 30};
    const Vec2 touching{20, 20};
    const Vec2 goal{35, 5};
    std::vector<Vec2> route{first, touching, goal};
    REQUIRE(grid.clear(position, goal));
    REQUIRE_FALSE(grid.clear(position, touching));

    CHECK(distance(route_target(position, route, grid, {}), first) == 0);
    CHECK(route.size() == 3);
}

TEST_CASE("a blocked straight raster sweep falls back to checking each waypoint") {
    const auto grid = footprint_grid();
    const Vec2 position{5, 25};
    const Vec2 first{15, 25};
    const Vec2 hidden{35, 25};
    const Vec2 goal{45, 25};
    std::vector<Vec2> route{first, hidden, goal};
    REQUIRE_FALSE(grid.blocked(grid.cell(hidden)));
    REQUIRE_FALSE(grid.clear(position, goal));

    CHECK(distance(route_target(position, route, grid, {}), first) == 0);
    CHECK(route.size() == 3);
}

TEST_CASE("raster route shortcuts account for the swept footprint") {
    const auto grid = footprint_grid();
    const Vec2 position{5, 15};
    const Vec2 corner{35, 15};
    const Vec2 goal{35, 19};
    std::vector<Vec2> route{corner, goal};

    CHECK(distance(route_target(position, route, grid, {.half_extents = footprint}), corner) == 0);
    REQUIRE(route.size() == 2);
    CHECK(distance(route_target(position, route, grid, {}), goal) == 0);
    CHECK(route.size() == 1);
}

TEST_CASE("invalid raster route inputs leave the waypoints intact") {
    const auto grid = corridor_grid();
    const Vec2 position{5, 15}, corner{15, 15}, goal{25, 15};
    std::vector<Vec2> route{corner, goal};
    const auto infinity = std::numeric_limits<float>::infinity();

    CHECK_THROWS_AS(route_target(position, route, grid, {.lookahead = -1}), std::invalid_argument);
    CHECK_THROWS_AS(route_target(position, route, grid, {.lookahead = infinity}), std::invalid_argument);
    CHECK_THROWS_AS(route_target(position, route, grid, {.half_extents = {-1, 0}}), std::invalid_argument);
    CHECK_THROWS_AS(route_target({infinity, 0}, route, grid, {}), std::invalid_argument);
    REQUIRE(route.size() == 2);
    CHECK(distance(route.front(), corner) == 0);
    CHECK(distance(route.back(), goal) == 0);
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
    const auto tangent = tangent_steer(agent, {100, 20}, std::span(&neighbor, 1), 2, .6f, 8);
    CHECK_MESSAGE(finite(tangent), "Invalid tangent acceleration");
    CHECK_MESSAGE(length(tangent) <= .600001f, "Invalid tangent acceleration");
    CHECK_MESSAGE(length(tangent_steer(agent, agent.position, {}, 2, .6f, 8)) <= .600001f,
                  "Tangent arrival acceleration exceeded limit");
}

TEST_CASE("tangent steering rejects an unbounded horizon") {
    CHECK_THROWS_AS(tangent_steer(cruiser(), {100, 20}, {}, 2, .6f, 65), std::invalid_argument);
}

TEST_CASE("priority steering keeps a step that only rounding puts inside the extent") {
    // A neighbor creeps along y below the float spacing at y = 10,002, so every float gap rounds
    // to -2, inside the extent of 2.00001, while the exact gap leaves it after ten steps. The
    // loop over every pair finds (0, 19); without the slack the search would skip i = 0.
    auto settings = steering_settings(70);
    settings.prediction_steps = 32;
    settings.safety_multiplier = 1;
    Body mover = cruiser_at({0, 10000}, {0, 1});
    mover.half_extents = {1, 1};
    Body other = cruiser_at({20, 10002}, {-1, 1e-6f});
    other.half_extents = {1, 1.00001f};
    const Body neighbors[]{other};
    const Vec2 expected = brute_force_predictive(mover, neighbors, settings);
    const Vec2 actual = priority_steer(mover, mover.position, neighbors, {}, settings).predictive;
    INFO("actual (", actual.x, ", ", actual.y, "), expected (", expected.x, ", ", expected.y, ")");
    REQUIRE(expected != Vec2{});
    CHECK(actual == expected);
}

TEST_CASE("priority steering predicts the same first overlap as testing every pair of steps") {
    // Random scenes at Lab's scales: unit boxes, and i4sim's millimeters with a horizon of 32
    // steps; whole-number positions and speeds make exact ties between steps more likely.
    // Further scales use a fractional step, a non-default safety margin, negative and large
    // coordinates. The reference is compiled in this file, so both copies of the expression
    // must round alike: this holds without contraction into fused multiply-adds, as on the
    // targets CI builds.
    std::mt19937 rng(41);
    struct Scale {
        float base, position, extent, speed, step, safety;
        int steps;
        bool whole;
    };
    for (const Scale scale :
         {Scale{0, 40, 2, 2, 1, 1.025f, 8, false}, Scale{0, 40, 2, 2, 1, 1.025f, 8, true},
          Scale{0, 9000, 400, 12, 1, 1.025f, 32, false}, Scale{0, 9000, 400, 12, 1, 1.025f, 32, true},
          Scale{-500, 40, 2, 2, 0.1f, 1.5f, 24, false},
          Scale{1000000, 9000, 400, 12, 1.f / 3, 1.1f, 32, false}}) {
        CAPTURE(scale.base);
        CAPTURE(scale.step);
        auto settings = steering_settings(70);
        settings.max_speed = scale.speed;
        settings.prediction_steps = scale.steps;
        settings.prediction_step = scale.step;
        settings.safety_multiplier = scale.safety;
        std::uniform_real_distribution<float> position(0, scale.position),
            extent(scale.extent / 4, scale.extent), speed(-scale.speed, scale.speed);
        auto value = [&](std::uniform_real_distribution<float> &d) {
            const float v = d(rng);
            return scale.whole ? std::round(v) : v;
        };
        auto box = [&] {
            Body body;
            body.shape = Shape::box;
            body.position = {scale.base + value(position), scale.base + value(position)};
            body.half_extents = {std::max(1.f, value(extent)), std::max(1.f, value(extent))};
            body.velocity = {value(speed), value(speed)};
            // Some neighbors stand still or creep along one axis.
            if (rng() % 4 == 0)
                body.velocity = {};
            else if (rng() % 4 == 0)
                body.velocity.y = 1e-6f;
            return body;
        };
        int overlaps = 0;
        for (int scene = 0; scene < 300; ++scene) {
            Body mover = box();
            if (length(mover.velocity) == 0)
                mover.velocity = {1, 0};
            std::vector<Body> neighbors;
            for (int k = 0; k < 6; ++k) {
                Body other = box();
                // Keep them apart at the start, so the predictive term decides.
                if (collide(mover, other))
                    continue;
                neighbors.push_back(other);
            }
            const Vec2 expected = brute_force_predictive(mover, neighbors, settings);
            const Vec2 actual = priority_steer(mover, mover.position, neighbors, {}, settings).predictive;
            if (expected != Vec2{})
                ++overlaps;
            INFO("actual (", actual.x, ", ", actual.y, "), expected (", expected.x, ", ", expected.y, ")");
            CHECK(actual == expected);
        }
        // Enough scenes predict an overlap for the comparison to mean something.
        CHECK(overlaps >= 5);
    }
}

TEST_CASE("passing predicts an opposing conflict within the horizon") {
    const PassingApproach east{{0, 0}, {2, 3}, {1, 0}, {60, 0}};
    PassingApproach west{{40, 0}, {3, 2}, {-1, 0}, {-20, 0}};
    CHECK(passing_conflict_time(east, west, 2, 10, 1) == 10);
    CHECK_FALSE(passing_conflict_time(east, west, 2, 9, 1));
    west.position.y = 6; // Touching the extended footprint alone leaves room.
    CHECK_FALSE(passing_conflict_time(east, west, 2, 10, 1));
    west.position.y = 0;
    west.heading = east.heading;
    CHECK_FALSE(passing_conflict_time(east, west, 2, 10, 1));
    west.heading = {-1, 0};
    west.position.x = -40;
    CHECK_FALSE(passing_conflict_time(east, west, 2, 10, 1));
}

TEST_CASE("passing orders candidate lanes and leaves room to resume the trip") {
    const PassingApproach east{{0, 0}, {2, 3}, {1, 0}, {60, 0}};
    PassingApproach west{{40, 0}, {3, 2}, {-1, 0}, {-20, 0}};
    constexpr PassingMargins margins{1, 2, 3};
    const auto lanes = passing_lane_candidates(east, west, margins);
    for (const auto &candidate : lanes)
        REQUIRE(candidate);
    check_passing_point((*lanes[0])[0].entry, Vec2{2.5f, -4});
    check_passing_point((*lanes[0])[0].exit, Vec2{27, -4});
    check_passing_point((*lanes[0])[1].exit, Vec2{13, 3});
    check_passing_point((*lanes[1])[0].exit, Vec2{27, -8});
    check_passing_point((*lanes[2])[0].exit, Vec2{27, 0});
    check_passing_point((*lanes[3])[0].exit, Vec2{27, 4});
    west.destination.x = 12.5f;
    for (const auto &candidate : passing_lane_candidates(east, west, margins))
        CHECK_FALSE(candidate);
    west.destination.x = -20;
    west.position.x = 12;
    for (const auto &candidate : passing_lane_candidates(east, west, margins))
        CHECK_FALSE(candidate);
}

TEST_CASE("passing guard distinguishes crossing traffic from a blocked lane") {
    auto box = [](Vec2 position, Vec2 velocity = {}) {
        Body body;
        body.shape = Shape::box;
        body.position = position;
        body.velocity = velocity;
        body.half_extents = {1, 1};
        return body;
    };
    const PassingLaneGuard guard(box({0, 0}, {2, 0}), {{0, 0}, {30, 0}}, PassingPhase::traverse, {},
                                 {2, 1, 3, 2});
    check_passing_point(guard.braking_target(), Vec2{4, 0});
    const auto bounds = guard.query_bounds(2);
    check_passing_point(bounds.min, Vec2{-3, -3});
    check_passing_point(bounds.max, Vec2{33, 3});
    CHECK(guard.inspect(box({22, 0}), true) == PassingObstruction::blocking);
    CHECK(guard.inspect(box({22, 2}), true) == PassingObstruction::clear);
    CHECK(guard.inspect(box({-1, 0}, {1, 0}), false) == PassingObstruction::clear);
    CHECK(guard.inspect(box({6, 7}, {0, -2}), false) == PassingObstruction::crossing);
    CHECK(guard.inspect(box({11, 0}, {-1, 0}), false) == PassingObstruction::blocking);
    CHECK(guard.inspect(box({20, 0}, {-1, 0}), false) == PassingObstruction::clear);
}

TEST_CASE("passing guard covers the approach before the traversal starts") {
    Body mover;
    mover.shape = Shape::box;
    mover.half_extents = {1, 1};
    auto parked = mover;
    parked.position = {0.5f, 2};
    const PassingLane lane{{0, 4}, {30, 4}};
    const PassingPrediction prediction{2, 1, 3, 2};
    const PassingLaneGuard approach(mover, lane, PassingPhase::approach, {}, prediction);
    CHECK(approach.inspect(parked, true) == PassingObstruction::blocking);
    parked.position.x = 0;
    CHECK(approach.inspect(parked, true) == PassingObstruction::clear);
    parked.position.x = 0.5f;
    mover.position = lane.entry;
    const PassingLaneGuard traverse(mover, lane, PassingPhase::traverse, {}, prediction);
    CHECK(traverse.inspect(parked, true) == PassingObstruction::clear);
    CHECK_THROWS_AS(PassingLaneGuard(mover, lane, PassingPhase::approach, {}, {2, 1, 3, 0}),
                    std::invalid_argument);
}

TEST_CASE("passing guard reaches moving traffic along a short lane") {
    for (float scale : {1.f, 10000.f}) {
        CAPTURE(scale);
        Body participant;
        participant.shape = Shape::box;
        participant.half_extents = Vec2{0.0001f, 0.0001f} * scale;
        const PassingLane lane{{0, 0}, {0.00005f * scale, 0}};
        const PassingLaneGuard guard(participant, lane, PassingPhase::traverse, {},
                                     {0.00004f * scale, scale, 1, 1});
        auto neighbor = participant;
        neighbor.position = {0.00023f * scale, 0};
        neighbor.velocity = {0, 0.000001f * scale};
        CHECK(guard.inspect(neighbor, false) == PassingObstruction::crossing);
    }
}
