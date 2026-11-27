#pragma once
#include <kinetra/simulation/physics.h>

#include <algorithm>

namespace kinetra::detail {
/// An axis-aligned box centered on the body's position that covers its bounds, with the body's
/// velocity: the footprint by which a mover of any shape takes part in passing plans, stall
/// detection and the right of way, so the targets a caller steers the body's position to are
/// the footprint's targets. One definition, so the navigation contract's footprint is the same
/// everywhere.
inline Body footprint(const Body &body) {
    const auto area = bounds(body);
    Body box;
    box.shape = Shape::box;
    box.motion = Motion::controlled;
    box.position = body.position;
    box.velocity = body.velocity;
    box.half_extents = {std::max(body.position.x - area.min.x, area.max.x - body.position.x),
                        std::max(body.position.y - area.min.y, area.max.y - body.position.y)};
    return box;
}
} // namespace kinetra::detail
