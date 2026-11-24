#include <algorithm>
#include <kinetra/simulation/spatial_grid.h>
#include <stdexcept>
#include <utility>

namespace kinetra {

namespace {
// Upper bound of the dense storage, 32 MB of cell offsets; larger areas use the hash map.
constexpr std::int64_t max_dense_cells = std::int64_t(1) << 22;
// The dense storage may hold up to this many empty cells per entry, plus a flat allowance for
// small grids: its offsets then cost at most a few times the entries themselves, and a grid of
// a few hundred boxes over a compact area always qualifies.
constexpr std::int64_t dense_cells_per_entry = 4;
constexpr std::int64_t dense_extra_cells = 1024;
} // namespace

SpatialGrid::SpatialGrid(float size) : cell_size_(size) {
    if (!std::isfinite(size) || size <= 0)
        throw std::invalid_argument("Grid cell size must be finite and positive");
}

SpatialGrid::CellRange SpatialGrid::cells(Aabb a) const {
    if (!finite(a.min) || !finite(a.max) || a.min.x > a.max.x || a.min.y > a.max.y)
        throw std::invalid_argument("Invalid spatial query bounds");
    const double x0 = std::floor(double(a.min.x) / cell_size_);
    const double y0 = std::floor(double(a.min.y) / cell_size_);
    const double x1 = std::floor(double(a.max.x) / cell_size_);
    const double y1 = std::floor(double(a.max.y) / cell_size_);
    // Bound index conversion and per-query work before integer conversion/allocation.
    if (std::min(x0, y0) < -1000000000 || std::max(x1, y1) > 1000000000 ||
        (x1 - x0 + 1) * (y1 - y0 + 1) > 100000)
        throw std::invalid_argument("Spatial bounds exceed grid limits");
    return {int(x0), int(y0), int(x1), int(y1)};
}

std::uint64_t SpatialGrid::key(int x, int y) {
    return (std::uint64_t(std::uint32_t(x)) << 32) | std::uint32_t(y);
}

void SpatialGrid::rebuild(std::span<const Aabb> bounds) {
    // Validate every box before the index changes, so a throw leaves it intact.
    std::vector<CellRange> ranges;
    ranges.reserve(bounds.size());
    std::int64_t entries = 0;
    CellRange extent{0, 0, -1, -1};
    for (const auto &b : bounds) {
        const auto r = cells(b);
        if (ranges.empty())
            extent = r;
        extent = {std::min(extent.min_x, r.min_x), std::min(extent.min_y, r.min_y),
                  std::max(extent.max_x, r.max_x), std::max(extent.max_y, r.max_y)};
        entries += std::int64_t(r.max_x - r.min_x + 1) * (r.max_y - r.min_y + 1);
        ranges.push_back(r);
    }
    // Only an allocation can fail from here on. It leaves an empty index rather than one whose
    // cell storage belongs to other boxes.
    try {
        bounds_.assign(bounds.begin(), bounds.end());
        ranges_ = std::move(ranges);
        extent_ = extent;
        const std::int64_t width = std::int64_t(extent.max_x) - extent.min_x + 1;
        const std::int64_t height = std::int64_t(extent.max_y) - extent.min_y + 1;
        // A dense array of cells when the occupied area is small next to the entries, so its
        // memory stays proportional to the boxes; a hash map of cells otherwise.
        dense_ = !ranges_.empty() && width * height <= dense_cells_per_entry * entries + dense_extra_cells &&
                 width * height <= max_dense_cells;
        if (dense_)
            index_dense(int(width), std::size_t(width * height), std::size_t(entries));
        else
            index_sparse();
    } catch (...) {
        bounds_.clear();
        ranges_.clear();
        extent_ = {0, 0, -1, -1};
        dense_ = false;
        starts_.clear();
        ids_.clear();
        buckets_.clear();
        throw;
    }
}

void SpatialGrid::index_dense(int width, std::size_t cell_count, std::size_t entries) {
    width_ = width;
    // Count the boxes of each cell into starts_[c + 1], sum them into the first entry of each
    // cell, use those as fill positions, then shift them back by one cell.
    starts_.assign(cell_count + 1, 0);
    for (const auto &r : ranges_)
        for (int y = r.min_y; y <= r.max_y; ++y)
            for (int x = r.min_x; x <= r.max_x; ++x)
                ++starts_[cell_index(x, y) + 1];
    for (std::size_t c = 1; c < starts_.size(); ++c)
        starts_[c] += starts_[c - 1];
    ids_.resize(entries);
    // Filled in index order, so every cell lists its boxes ascending.
    for (std::size_t i = 0; i < ranges_.size(); ++i) {
        const auto &r = ranges_[i];
        for (int y = r.min_y; y <= r.max_y; ++y)
            for (int x = r.min_x; x <= r.max_x; ++x)
                ids_[starts_[cell_index(x, y)]++] = i;
    }
    std::copy_backward(starts_.begin(), starts_.end() - 1, starts_.end());
    starts_[0] = 0;
}

void SpatialGrid::index_sparse() {
    starts_.clear();
    ids_.clear();
    // Keep the storage of the cells in use: a grid rebuilt every tick over boxes that barely
    // move would otherwise free and allocate every bucket again. A cell that stayed empty
    // for a whole rebuild is dropped, so the map tracks the cells of the last two rebuilds;
    // dense rebuilds in between leave the map alone.
    for (auto it = buckets_.begin(); it != buckets_.end();) {
        if (it->second.empty()) {
            it = buckets_.erase(it);
        } else {
            it->second.clear();
            ++it;
        }
    }
    for (std::size_t i = 0; i < ranges_.size(); ++i) {
        const auto &r = ranges_[i];
        for (int y = r.min_y; y <= r.max_y; ++y)
            for (int x = r.min_x; x <= r.max_x; ++x)
                buckets_[key(x, y)].push_back(i);
    }
}

std::vector<std::size_t> SpatialGrid::query(Aabb area) const {
    std::vector<std::size_t> result;
    query(area, result);
    return result;
}

void SpatialGrid::query(Aabb area, std::vector<std::size_t> &result) const {
    const auto q = cells(area);
    result.clear();
    // Only cells that hold boxes can contribute.
    const int x0 = std::max(q.min_x, extent_.min_x), x1 = std::min(q.max_x, extent_.max_x);
    const int y0 = std::max(q.min_y, extent_.min_y), y1 = std::min(q.max_y, extent_.max_y);
    if (ranges_.empty() || x0 > x1 || y0 > y1)
        return;
    // A box that spans several cells of the query is reported only from the first of them,
    // its lowest x and y in the query, so no result repeats.
    auto visit = [&](int x, int y, std::size_t id) {
        const auto &r = ranges_[id];
        if (x == std::max(r.min_x, q.min_x) && y == std::max(r.min_y, q.min_y) && bounds_[id].overlaps(area))
            result.push_back(id);
    };
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (dense_) {
                const auto c = cell_index(x, y);
                for (auto k = starts_[c]; k < starts_[c + 1]; ++k)
                    visit(x, y, ids_[k]);
            } else if (const auto found = buckets_.find(key(x, y)); found != buckets_.end()) {
                for (auto id : found->second)
                    visit(x, y, id);
            }
        }
    // One cell lists its boxes ascending already.
    if (x0 != x1 || y0 != y1)
        std::sort(result.begin(), result.end());
}

} // namespace kinetra
