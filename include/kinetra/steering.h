#pragma once

#include "physics.h"

namespace kinetra {

struct Steering {
    float max_speed{5}, max_acceleration{12};
    float response_time{0.25f}, slow_radius{3}, stop_radius{0.15f};
    float lookahead{1}, clearance{0.3f};
    // Optional personal space; zero keeps separation disabled.
    float separation_radius{}, separation_weight{1};
};

/// Returns bounded acceleration, not a position change. Arrival reduces desired speed
/// inside slow_radius; avoidance predicts a swept circle against nearby obstacle bounds.
/// Supply neighbors from a SpatialGrid query; boxes use expanded local-space bounds.
/// This is local steering, not path planning; concave traps can require waypoints.
Vec2 steer(const Body &agent, Vec2 target, std::span<const Body> obstacles, const Steering &settings = {});

} // namespace kinetra
