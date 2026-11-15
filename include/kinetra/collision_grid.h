#pragma once
#include "physics.h"
#include <span>

namespace kinetra {
/// Row-major occupancy; nonzero cells are solid. Segment queries also block outside the view.
struct CollisionGrid {
    std::span<const int> cells;
    int rows{}, columns{};
    float cell_size{1};
    Vec2 origin{};
};
/// Resolve overlapping solid cells using engine circle/box contact response.
void collide_grid(Body &body, const CollisionGrid &grid);
/// Return the first blocked fraction; handles stationary and corner-crossing segments.
std::optional<float> segment_grid(Vec2 start, Vec2 end, const CollisionGrid &grid);
} // namespace kinetra
