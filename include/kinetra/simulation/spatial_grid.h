#pragma once

#include <cstddef>
#include <cstdint>
#include <kinetra/simulation/math.h>
#include <span>
#include <unordered_map>
#include <vector>

namespace kinetra {

/// Rebuildable AABB index. Results are sorted unique input indices whose bounds overlap
/// the query. Negative coordinates and objects spanning multiple cells are supported.
///
/// Rebuild it whenever the bounds change; there is no incremental update. A box may span at
/// most 100,000 cells and its cell coordinates must stay within ±10^9, which keeps the cost
/// of one bad box bounded.
class SpatialGrid {
  public:
    /// `cell_size` in the caller's units; throws std::invalid_argument unless it is finite
    /// and positive. Choose it near the typical object size.
    explicit SpatialGrid(float cell_size = 4);
    /// Replaces the index with a copy of `bounds`; index i in later queries is `bounds[i]`.
    /// Throws std::invalid_argument for a nonfinite, inverted or oversized box, and then
    /// leaves the previous index intact.
    void rebuild(std::span<const Aabb> bounds);
    /// Indices of the stored boxes that overlap `area` (touching counts), sorted and
    /// unique. Throws std::invalid_argument for a nonfinite, inverted or oversized area.
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
