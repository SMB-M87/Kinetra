#pragma once
#include <kinetra/simulation/math.h>
#include <span>
#include <vector>

namespace kinetra {
/// Eight-neighbor A* with rectangular cells and nonnegative congestion costs.
/// Blocked cells and both side cells of a diagonal are excluded. Coordinates outside
/// the grid are rejected; unreachable endpoints return an empty path.
///
/// Positions are in the caller's world units with the grid's corner at (0, 0); cell ids
/// are row-major, `y * width() + x`. An edge costs its length times (1 + the weight of the
/// cell it enters). All cells start free with weight zero.
class WeightedGrid {
  public:
    /// Covers `dimension` with whole cells of `cell_size`; a partial last row or column is
    /// dropped. Throws std::invalid_argument for nonfinite or nonpositive sizes, a dimension
    /// smaller than one cell, more than 100,000 cells per axis or more than a million cells.
    WeightedGrid(Vec2 dimension, Vec2 cell_size);
    /// Number of columns.
    int width() const { return width_; }
    /// Number of rows.
    int height() const { return height_; }
    /// Id of the cell containing `position`, or -1 when it is outside the grid or nonfinite.
    int cell(Vec2 position) const;
    /// Center of cell `cell`; throws std::out_of_range for an invalid id.
    Vec2 center(int cell) const;
    /// Whether `cell` is blocked; ids outside the grid count as blocked.
    bool blocked(int cell) const;
    /// Blocks or frees `cell`; throws std::out_of_range for an invalid id.
    void block(int cell, bool blocked = true);
    /// Replaces all weights at once, indexed by cell id. Throws std::invalid_argument, and
    /// keeps the old weights, unless there is one finite nonnegative value per cell.
    void weights(std::span<const float> values);
    /// Weight of `cell`; throws std::out_of_range for an invalid id.
    float weight(int cell) const;
    /// Exact swept axis-aligned footprint; touching a surface is permitted.
    ///
    /// True when a rectangle of `half_extents` moving from `start` to `goal` stays inside the
    /// grid and enters no blocked cell. Throws std::invalid_argument for nonfinite points or
    /// negative half extents.
    bool clear(Vec2 start, Vec2 goal, Vec2 half_extents = {}) const;
    /// Cheapest route for a footprint of `half_extents` from `start` to `goal`. Returns
    /// waypoints after `start`: cell centers, then the exact `goal`. Empty when an endpoint
    /// is nonfinite, blocked or outside the grid, or no route exists. Throws
    /// std::invalid_argument for negative half extents.
    std::vector<Vec2> path(Vec2 start, Vec2 goal, Vec2 half_extents = {}) const;
    /// Merged boundary edges of the free-cell union, including outer boundaries.
    std::vector<Segment> borders() const;

  private:
    int width_, height_;
    Vec2 cell_size_;
    std::vector<bool> blocked_;
    std::vector<float> weights_;
};
} // namespace kinetra
