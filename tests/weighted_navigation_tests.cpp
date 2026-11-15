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
        require(forces.border.x < 0 && forces.collision.x < 0 && forces.acceleration.x < 0,
                "Safety priority lost to seek");
        require(length(forces.acceleration) <= settings.max_acceleration + 1e-6f,
                "Acceleration limit exceeded");
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
