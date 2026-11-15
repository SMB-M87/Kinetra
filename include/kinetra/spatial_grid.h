#pragma once

#include "math.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace kinetra {

/// Rebuildable AABB index. Results are sorted unique input indices whose bounds overlap
/// the query. Negative coordinates and objects spanning multiple cells are supported.
class SpatialGrid {
  public:
    explicit SpatialGrid(float cell_size = 4);
    void rebuild(std::span<const Aabb> bounds);
    std::vector<std::size_t> query(Aabb area) const;

  private:
    struct CellRange {
        int min_x, min_y, max_x, max_y;
    };
    CellRange cells(Aabb area) const;
    static std::uint64_t key(int x, int y);
    float cell_size_;
    std::vector<Aabb> bounds_;
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets_;
};

} // namespace kinetra
