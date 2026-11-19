#pragma once

#include "math.h"
#include <cstdint>
#include <span>

namespace kinetra {

/// Immutable avoidance envelope; use bounding_radius(Body) for convex bodies.
/// IDs must be unique within a query. Noncooperative neighbors keep their velocity;
/// reciprocal neighbors are assumed to run the same avoidance model concurrently.
///
/// Positions in world units; velocities in units per caller time unit (the same unit as
/// `dt` in avoid_collisions()).
struct AvoidanceAgent {
    /// Stable identifier; also fixes the order in which constraints are built.
    std::uint64_t id{};
    /// Current center and velocity.
    Vec2 position{}, velocity{};
    /// Positive radius of the circular envelope.
    float radius{0.5f};
    /// Whether this agent shares avoidance with the caller. False assigns the full
    /// avoidance responsibility to the caller.
    bool cooperative{true};
    /// Optional fixed axis-aligned rectangle. Both components must be positive,
    /// or both zero for a circle. Radius must enclose the rectangle for mixed pairs.
    Vec2 half_extents{};
};
/// Limits and tuning of avoid_collisions(); all values finite and nonnegative, `horizon`
/// positive.
struct AvoidanceSettings {
    /// Speed cap and acceleration cap, per caller time unit.
    float max_speed{4}, max_acceleration{12};
    /// `horizon`: how far ahead conflicts are predicted, in time units. `clearance`: extra
    /// gap added to every pair of envelopes, in world units.
    float horizon{1.5f}, clearance{0.1f};
    /// Fraction of preferred speed (capped at max_speed) added to the right-hand
    /// passing direction near forward neighbors. Zero disables this preference.
    float passing_bias{0.05f};
};
/// Selected motion for the next update.
struct AvoidanceResult {
    /// New velocity after one step of `acceleration`, and the bounded acceleration itself.
    Vec2 velocity{}, acceleration{};
    /// False when no velocity satisfies every constraint within the limits.
    bool feasible{true};
    /// Largest residual half-plane violation in velocity units; zero when satisfied.
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
