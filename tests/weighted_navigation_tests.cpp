#include <iostream>
#include <kinetra/steering.h>
#include <kinetra/weighted_grid.h>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        using namespace kinetra;
        WeightedGrid grid({50, 30}, {10, 10});
        auto route = grid.path({5, 15}, {45, 15});
        require(route.size() == 4 && distance(route.back(), {45, 15}) == 0, "Free path endpoint lost");
        std::vector<float> weights(15);
        weights[7] = 100;
        grid.weights(weights);
        route = grid.path({5, 15}, {45, 15});
        for (auto point : route)
            require(grid.cell(point) != 7, "Congestion cost ignored");
        grid.block(1);
        grid.block(5);
        require(grid.path({5, 5}, {15, 15}).empty(), "Diagonal cut a blocked corner");
        require(grid.path({-1, 5}, {15, 15}).empty(), "Outside start accepted");
        require(!grid.borders().empty(), "Missing free-space borders");
        bool rejected = false;
        try {
            weights[0] = -1;
            grid.weights(weights);
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        require(rejected && grid.weight(0) == 0, "Invalid weights partially mutated the grid");
        {
            WeightedGrid footprint({50, 50}, {10, 10});
            require(footprint.borders().size() == 4, "Raster seams remain in straight walls");
            footprint.block(12);
            require(footprint.path({5, 5}, {19, 25}).size() > 0, "Point route unexpectedly rejected");
            require(footprint.path({5, 5}, {19, 25}, {2, 2}).empty(),
                    "Parking footprint overlaps a blocked neighboring cell");
            require(footprint.path({5, 5}, {1, 35}, {2, 2}).empty(),
                    "Parking footprint crosses the outer boundary");
            require(!footprint.clear({15, 15}, {35, 35}, {2, 2}),
                    "Swept footprint cut a blocked corner");
            require(footprint.clear({18, 15}, {18, 35}, {2, 2}),
                    "Clear wall-tangent footprint rejected");
            const auto safe = footprint.path({5, 25}, {45, 25}, {2, 2});
            require(!safe.empty(), "Footprint planner failed to detour");
            Vec2 previous{5, 25};
            for (auto point : safe) {
                require(footprint.clear(previous, point, {2, 2}), "Unsafe footprint waypoint link");
                previous = point;
            }
            footprint.block(7);
            require(footprint.borders().size() == 8, "Collinear obstacle boundaries were not merged");
        }
        Body a;
        a.shape = Shape::box;
        a.half_extents = {5, 5};
        a.position = {20, 20};
        a.velocity = {2, 0};
        PrioritySteering settings;
        settings.arrival_radius = 10;
        auto forces = priority_steer(a, {100, 20}, {}, {}, settings);
        require(length(forces.acceleration) == 0, "Cruising speed incorrectly accelerated");
        forces = priority_steer(a, a.position, {}, {}, settings);
        require(forces.acceleration.x < 0, "Arrival did not brake");
        Body neighbor = a;
        neighbor.position = {28, 20};
        neighbor.velocity = {};
        const Segment edge{{25, 0}, {25, 100}};
        forces = priority_steer(a, {100, 20}, std::span(&neighbor, 1), std::span(&edge, 1), settings);
        require(forces.border.x <= 0 && forces.collision.x < 0 && forces.acceleration.x < 0,
                "Safety priority lost to seek");
        require(length(forces.acceleration) <= settings.max_acceleration + 1e-6f,
                "Acceleration limit exceeded");
        {
            Body sliding = a;
            const Segment wall{{0, -100}, {0, 100}};
            sliding.position = {5, 20};
            sliding.velocity = {0, .2f};
            const auto along = constrain_borders(sliding, {-.2f, .1f}, std::span(&wall, 1), settings);
            require(along.x == 0 && std::abs(along.y - .1f) < 1e-6f,
                    "Wall safety discarded tangential steering");
            Vec2 previous{};
            bool first = true;
            for (float gap : {-.01f, -.001f, 0.f, .001f, .01f}) {
                sliding.position.x = 5 + gap;
                const auto value = constrain_borders(sliding, {-.2f, .1f}, std::span(&wall, 1), settings);
                require(finite(value) && length(value) <= settings.max_acceleration + 1e-6f,
                        "Wall reaction exceeded acceleration bound");
                if (!first) require(distance(previous, value) < .01f, "Wall reaction jumps at contact");
                first = false;
                previous = value;
            }
            sliding.position.x = 5;
            sliding.velocity = {};
            require(length(constrain_borders(sliding, {}, std::span(&wall, 1), settings)) == 0,
                    "A parked touching body was repelled");
            sliding.position = {25, 20};
            sliding.velocity = {-2, 0};
            for (int tick = 0; tick < 400; ++tick) {
                const Vec2 desired = limited(Vec2{5.25f - sliding.position.x, .1f} - sliding.velocity,
                                             settings.max_acceleration);
                const auto acceleration = constrain_borders(sliding, desired, std::span(&wall, 1), settings);
                sliding.velocity = limited(sliding.velocity + acceleration, settings.max_speed);
                sliding.position += sliding.velocity;
                require(sliding.position.x >= 5 - 1e-5f, "Normal braking crossed the wall");
                require(length(acceleration) <= settings.max_acceleration + 1e-5f,
                        "Normal braking exceeded the acceleration budget");
            }
            require(std::abs(sliding.position.x - 5.25f) < 1e-3f,
                    "Wall braking prevented convergence to a close parking goal");
            const Segment corner[] = {{{0, -100}, {0, 100}}, {{-100, 0}, {100, 0}}};
            sliding.position = {5.1f, 5.1f};
            sliding.velocity = {-.05f, -.05f};
            const auto constrained = constrain_borders(sliding, {-.2f, -.2f}, corner, settings);
            require(constrained.x >= 0 && constrained.y >= 0, "Corner approach did not brake");
        }
        neighbor.position = {40, 20};
        neighbor.velocity = {-2, 0};
        forces = priority_steer(a, {100, 20}, std::span(&neighbor, 1), {}, settings);
        require(forces.predictive.x < 0 && forces.acceleration.x < 0,
                "Approaching collision was not predicted");
        neighbor.position = a.position;
        forces = priority_steer(a, {100, 20}, std::span(&neighbor, 1), {}, settings);
        require(finite(forces.acceleration) && length(forces.collision) > 0,
                "Coincident bodies were not handled");
        const auto tangent = tangent_steer(a, {100,20}, std::span(&neighbor,1));
        require(finite(tangent) && length(tangent) <= .600001f, "Invalid tangent acceleration");
        require(length(tangent_steer(a, a.position, {})) <= .600001f,
                "Tangent arrival acceleration exceeded limit");
        rejected = false;
        try { tangent_steer(a, {100,20}, {}, 2, .6f, 65); }
        catch (const std::invalid_argument &) { rejected = true; }
        require(rejected, "Tangent steering accepted unbounded horizon");
        std::cout << "PASS weighted navigation and priority steering\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
