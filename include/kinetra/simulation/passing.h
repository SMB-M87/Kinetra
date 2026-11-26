#pragma once

#include <kinetra/simulation/physics.h>

#include <array>
#include <cstddef>
#include <optional>

namespace kinetra {
/// An axis-aligned footprint, route heading and final goal in world coordinates.
struct PassingApproach {
    /// Center and positive half extents in world units.
    Vec2 position, half_extents;
    /// Route direction, normalized or zero, and final goal in world coordinates.
    Vec2 heading, destination;
};

/// Predict the time of closest approach for opposing headings at the same speed.
/// Headings must be normalized or zero. Returns a time in the caller's time unit
/// when the footprints would overlap with the given gap within the horizon.
/// Supply finite positive speed and finite nonnegative horizon and gap.
std::optional<float> passing_conflict_time(const PassingApproach &first, const PassingApproach &second,
                                           float speed, float horizon, float gap);

/// Geometry margins in world units. All three values must be finite and nonnegative.
struct PassingMargins {
    /// Extra space per side of the lane divider and beyond each exit before the destination.
    float gap;
    /// Extra distance beyond the midpoint and the two bodies at each exit.
    float exit_offset;
    /// Extra longitudinal distance required between the participants at formation.
    float formation_room;
};

/// Targets for one participant. Reach entry before starting the leg to exit.
struct PassingLane {
    Vec2 entry, exit;
};

/// Lane targets in the order in which the participants were supplied.
using PassingLanes = std::array<PassingLane, 2>;

/// For a conflicting pair, try each side, then shifts in either direction. Empty entries
/// lack formation room or run past a destination. Headings must be normalized or zero.
/// Callers check the two legs of each lane against terrain, obstacles and reservations.
std::array<std::optional<PassingLanes>, 6>
passing_lane_candidates(const PassingApproach &first, const PassingApproach &second, PassingMargins margins);

/// Result of looking at one neighbor from a participant's current position.
enum class PassingObstruction {
    clear,
    /// A moving neighbor is predicted to occupy a reachable part of the lane.
    crossing,
    /// A stationary neighbor occupies a remaining leg, or an oncoming one is predicted in it.
    blocking,
};

/// Passing has an approach stage followed by travel from entry to exit.
enum class PassingPhase { approach, traverse };

/// Prediction parameters in consistent caller-selected time units.
struct PassingPrediction {
    /// Positive finite speed and acceleration limits.
    float speed, acceleration;
    /// Nonnegative horizon and positive sample interval in whole time units.
    int horizon, sample_interval;
};

/// Classifies neighbors ahead of one participant on its remaining legs. The guard holds
/// a snapshot and borrows no objects. The caller excludes the partner and decides which
/// moving neighbors have priority before checking them. It owns the response and its timing.
class PassingLaneGuard {
  public:
    /// `participant` must be an axis-aligned box with finite position, velocity and extent.
    /// `clearance` adds to its half extents. Throws std::invalid_argument for invalid prediction.
    PassingLaneGuard(const Body &participant, PassingLane lane, PassingPhase phase, Vec2 clearance,
                     PassingPrediction prediction);

    /// Bounds of the remaining legs, with the inflated footprint and extra search margin.
    Aabb query_bounds(float margin) const;

    /// Classify a neighbor with an axis-aligned footprint. Stationary neighbors are checked
    /// against all remaining legs. Moving neighbors are sampled up to the prediction horizon.
    PassingObstruction inspect(const Body &neighbor, bool stationary) const;

    /// Stop target along current velocity, using a conservative speed²/acceleration distance.
    Vec2 braking_target() const;

  private:
    Vec2 position_, velocity_, extent_, axis_;
    PassingPrediction prediction_;
    std::array<Segment, 2> legs_{}, near_{};
    std::size_t leg_count_{}, near_count_{};
};
} // namespace kinetra
