#pragma once
#include "physics.h"
#include <span>

namespace kinetra {
/// Row-major occupancy; nonzero cells are solid. Segment queries also block outside the view.
///
/// A borrowed view: `cells` must stay alive while a query runs, and nothing is copied.
/// Cell (row, column) covers `origin + (column, row) * cell_size` to one cell further, in
/// world units; row 0 is at `origin.y`. Every query throws std::invalid_argument unless
/// rows and columns are positive, `cells` holds exactly rows * columns values, and
/// `cell_size` and `origin` are finite with a positive cell size.
struct CollisionGrid {
    /// Occupancy, row by row; nonzero means solid.
    std::span<const int> cells;
    /// Grid dimensions in cells.
    int rows{}, columns{};
    /// Edge length of one square cell, in world units.
    float cell_size{1};
    /// World position of the corner of cell (0, 0).
    Vec2 origin{};
};
/// Pushes `body` out of every overlapping solid cell, using engine circle/box contact
/// response against frictionless fixed boxes, in eight passes. Cells outside the grid
/// are not solid here. Throws std::invalid_argument for an invalid body or grid.
void collide_grid(Body &body, const CollisionGrid &grid);
/// Return the first blocked fraction; handles stationary and corner-crossing segments.
///
/// The fraction lies in [0, 1] along the segment from `start` to `end`; zero when `start` is
/// in a solid cell or outside the grid, std::nullopt when the whole segment is free.
/// Throws std::invalid_argument for nonfinite endpoints or an invalid grid.
std::optional<float> segment_grid(Vec2 start, Vec2 end, const CollisionGrid &grid);
} // namespace kinetra
