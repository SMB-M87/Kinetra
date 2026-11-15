#pragma once

#include "physics.h"

namespace kinetra {

struct Steering {
    float max_speed{5}, max_acceleration{12};
    float response_time{0.25f}, slow_radius{3}, stop_radius{0.15f};
    float lookahead{1}, clearance{0.3f};
    // Optional personal space; zero keeps separation disabled.
    float separation_radius{}, separation_weight{1};
    // Use the exact Minkowski box for pairs of axis-aligned boxes.
    bool rectangular_clearance{};
};

/// Returns bounded acceleration, not a position change. Arrival reduces desired speed
/// inside slow_radius; avoidance predicts a swept circle against nearby obstacle bounds.
/// Supply neighbors from a SpatialGrid query; boxes use expanded local-space bounds.
/// rectangular_clearance uses summed half extents for axis-aligned box pairs only;
/// rotated boxes and all other shape pairs retain the conservative circular envelope.
/// This is local steering, not path planning; concave traps can require waypoints.
Vec2 steer(const Body &agent, Vec2 target, std::span<const Body> obstacles, const Steering &settings = {});

} // namespace kinetra

namespace kinetra {
/// Axis-aligned rectangular steering, expressed in caller-selected time units.
/// Returns bounded acceleration; integration remains the caller's responsibility.
struct PrioritySteering {
    float max_speed{2}, max_acceleration{0.6f}, arrival_radius{70};
    float border_distance{2}, border_strength{10}, border_lookahead{2};
    int prediction_steps{8};
    float prediction_step{1}, safety_multiplier{1.025f};
};
struct SteeringForces {
    Vec2 border{}, collision{}, predictive{}, seek{}, acceleration{};
};
/// Priority: borders and existing overlaps, then predicted collisions, then arrival.
/// Neighbors exclude the agent. Borders must be horizontal or vertical segments.
SteeringForces priority_steer(const Body &agent, Vec2 target, std::span<const Body> neighbors,
                              std::span<const Segment> borders, const PrioritySteering &settings = {},
                              bool arrival = true);
} // namespace kinetra

namespace kinetra {
/// Experimental tangent/spring avoidance translated from i4sim RVODeadlock.
/// Returns acceleration only; this is not a collision-free velocity solver.
Vec2 tangent_steer(const Body &agent, Vec2 target, std::span<const Body> neighbors,
                   float max_speed = 2, float max_force = 0.6f, float horizon = 8,
                   bool arrival = true);
}
