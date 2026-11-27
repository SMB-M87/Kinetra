#pragma once

#include <kinetra/simulation/physics.h>

namespace kinetra {

/// Tuning of steer(), in world units and seconds. Every value must be finite and
/// nonnegative, `response_time` positive and `slow_radius` larger than `stop_radius`.
struct Steering {
    /// Top speed (units per second) and acceleration cap (units per second squared).
    float max_speed{5}, max_acceleration{12};
    /// `response_time`: seconds to reach the desired velocity. Arrival slows down linearly
    /// from `slow_radius` and reaches zero speed at `stop_radius` (both in world units).
    float response_time{0.25f}, slow_radius{3}, stop_radius{0.15f};
    /// `lookahead`: prediction horizon in seconds. `clearance`: extra gap kept from
    /// obstacles, in world units.
    float lookahead{1}, clearance{0.3f};
    /// Optional personal space; zero keeps separation disabled.
    float separation_radius{}, separation_weight{1};
    /// Use the exact Minkowski box for pairs of axis-aligned boxes.
    bool rectangular_clearance{};
};

/// Returns bounded acceleration, not a position change. Arrival reduces desired speed
/// inside slow_radius; avoidance predicts a swept circle against nearby obstacle bounds.
/// Supply neighbors from a SpatialGrid query; boxes use expanded local-space bounds.
/// rectangular_clearance uses summed half extents for axis-aligned box pairs only;
/// rotated boxes and all other shape pairs retain the conservative circular envelope.
/// This is local steering, not path planning; concave traps can require waypoints.
///
/// Leave `agent` itself out of `obstacles`: it would be avoided like any other body.
/// Throws std::invalid_argument for an invalid body,
/// a nonfinite target or settings outside the ranges documented on Steering.
Vec2 steer(const Body &agent, Vec2 target, std::span<const Body> obstacles, const Steering &settings = {});

} // namespace kinetra

namespace kinetra {
/// Axis-aligned rectangular steering, expressed in caller-selected time units.
/// Returns bounded acceleration; integration remains the caller's responsibility.
///
/// Every float must be finite and positive, and `prediction_steps` lie in [0, 64].
/// Supply tuning for the caller's distance and time scales.
struct PrioritySteering {
    /// Speed per time unit and acceleration per squared time unit. Seek slows down
    /// inside `arrival_radius` when arrival is enabled.
    float max_speed{}, max_acceleration{}, arrival_radius{};
    /// Wall tuning: `border_distance` and `border_strength` shape the recovery from
    /// penetration, `border_lookahead` damps the approach speed toward a wall.
    float border_distance{}, border_strength{}, border_lookahead{};
    /// Number of future steps compared when predicting a collision with a neighbor.
    int prediction_steps{};
    /// `prediction_step`: time units per predicted step. `safety_multiplier`: scale on the
    /// summed half extents that counts as a predicted collision.
    float prediction_step{}, safety_multiplier{};
};
/// The forces priority_steer() considered, for diagnostics and debug drawing.
struct SteeringForces {
    /// `border`: correction applied by constrain_borders(). `collision`: overlap recovery.
    /// `predictive`: average push away from predicted collisions. `seek`: arrival toward the
    /// target. `acceleration`: the final capped and border-constrained result.
    Vec2 border{}, collision{}, predictive{}, seek{}, acceleration{};
};
/// Constrain next-step normal velocity near borders while retaining tangential steering.
/// Uses caller-selected time units with one integration step per call. Overlaps receive
/// bounded recovery; borders have no passive repulsion at rest outside their surface.
/// Beside the end of a segment, the face the agent would reach last at its current
/// velocity constrains it, so an agent sliding flush along one wall passes the end of
/// another that meets it at a convex corner. A face counts as reached only within
/// `max_speed / max_acceleration + border_lookahead` time units; when neither face is,
/// the one with the larger gap constrains the agent, so a residual drift toward a face far
/// away cannot brake a move along the other.
///
/// `agent` must be an unrotated box. Throws std::invalid_argument for another body, a
/// nonfinite acceleration, a border that is not horizontal or vertical, or settings
/// outside the ranges documented on PrioritySteering.
Vec2 constrain_borders(const Body &agent, Vec2 acceleration, std::span<const Segment> borders,
                       const PrioritySteering &settings);
/// Priority: existing overlaps, then predicted collisions, then arrival; borders
/// constrain the result. border reports the applied acceleration correction.
/// Neighbors exclude the agent. Borders must be horizontal or vertical segments.
///
/// `agent` and every neighbor must be unrotated boxes. With `arrival` false the agent
/// keeps full speed at the target, for intermediate waypoints. Throws
/// std::invalid_argument for unsupported bodies, a nonfinite target or invalid settings.
SteeringForces priority_steer(const Body &agent, Vec2 target, std::span<const Body> neighbors,
                              std::span<const Segment> borders, const PrioritySteering &settings,
                              bool arrival = true);
} // namespace kinetra

namespace kinetra {
/// Experimental tangent/spring avoidance translated from i4sim RVODeadlock.
/// Returns acceleration only; this is not a collision-free velocity solver.
///
/// Neighbors within `horizon` steps of travel push the agent along the tangent of their
/// enclosing circle. `max_speed` and `max_force` are per caller time unit; with `arrival`
/// the agent slows down near the target. Throws std::invalid_argument for a nonfinite
/// target, nonpositive limits or a horizon above 64.
Vec2 tangent_steer(const Body &agent, Vec2 target, std::span<const Body> neighbors, float max_speed,
                   float max_force, float horizon, bool arrival = true);
} // namespace kinetra
