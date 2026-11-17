#pragma once

#include "math.h"
#include <cstdint>
#include <span>

namespace kinetra {

/// Immutable avoidance envelope; use bounding_radius(Body) for convex bodies.
/// IDs must be unique within a query. Noncooperative neighbors keep their velocity;
/// reciprocal neighbors are assumed to run the same avoidance model concurrently.
struct AvoidanceAgent {
    std::uint64_t id{};
    Vec2 position{}, velocity{};
    float radius{0.5f};
    bool cooperative{true};
    // Optional fixed axis-aligned rectangle. Both components must be positive,
    // or both zero for a circle. Radius must enclose the rectangle for mixed pairs.
    Vec2 half_extents{};
};
struct AvoidanceSettings {
    float max_speed{4}, max_acceleration{12};
    float horizon{1.5f}, clearance{0.1f};
    // Fraction of preferred speed (capped at max_speed) added to the right-hand
    // passing direction near forward neighbors. Zero disables this preference.
    float passing_bias{0.05f};
};
struct AvoidanceResult {
    Vec2 velocity{}, acceleration{};
    bool feasible{true};
    // Largest residual half-plane violation in velocity units; zero when satisfied.
    float violation{};
};

/// ORCA-style circle/fixed-axis rectangle local avoidance with explicit speed/acceleration constraints.
/// Use one immutable neighbor snapshot per update; neighbors exclude the agent.
/// dt and horizon use the same caller-selected time units as velocities/acceleration.
/// Infeasible constraints use bounded minimax relaxation and report feasible=false;
/// acceleration remains bounded. Existing overspeed may persist but cannot increase.
/// This is not path planning, rotational clearance or a general collision-free guarantee.
/// Throws invalid_argument for invalid parameters, duplicate IDs or nonfinite state.
AvoidanceResult avoid_collisions(const AvoidanceAgent &agent, Vec2 preferred_velocity,
                                 std::span<const AvoidanceAgent> neighbors, float dt,
                                 const AvoidanceSettings &settings = {});

} // namespace kinetra
