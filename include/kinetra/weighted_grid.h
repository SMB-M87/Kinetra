#pragma once
#include "math.h"
#include <span>
#include <vector>

namespace kinetra {
/// Eight-neighbor A* with rectangular cells and nonnegative congestion costs.
/// Blocked cells and both side cells of a diagonal are excluded. Coordinates outside
/// the grid are rejected; unreachable endpoints return an empty path.
class WeightedGrid {
  public:
    WeightedGrid(Vec2 dimension, Vec2 cell_size);
    int width() const { return width_; }
    int height() const { return height_; }
    int cell(Vec2 position) const;
    Vec2 center(int cell) const;
    bool blocked(int cell) const;
    void block(int cell, bool blocked = true);
    void weights(std::span<const float> values);
    float weight(int cell) const;
    std::vector<Vec2> path(Vec2 start, Vec2 goal) const;
    /// Boundary edges of the free-cell union, including outer boundaries.
    std::vector<Segment> borders() const;

  private:
    int width_, height_;
    Vec2 cell_size_;
    std::vector<bool> blocked_;
    std::vector<float> weights_;
};
} // namespace kinetra
