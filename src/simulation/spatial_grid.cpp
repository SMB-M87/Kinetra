#include <kinetra/spatial_grid.h>
#include <stdexcept>

namespace kinetra {

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
    for (const auto &b : bounds)
        (void)cells(b);
    bounds_.assign(bounds.begin(), bounds.end());
    buckets_.clear();
    for (std::size_t i = 0; i < bounds_.size(); ++i) {
        const auto r = cells(bounds_[i]);
        for (int y = r.min_y; y <= r.max_y; ++y)
            for (int x = r.min_x; x <= r.max_x; ++x)
                buckets_[key(x, y)].push_back(i);
    }
}

std::vector<std::size_t> SpatialGrid::query(Aabb area) const {
    const auto r = cells(area);
    std::vector<std::size_t> result;
    for (int y = r.min_y; y <= r.max_y; ++y)
        for (int x = r.min_x; x <= r.max_x; ++x) {
            const auto found = buckets_.find(key(x, y));
            if (found == buckets_.end())
                continue;
            for (auto id : found->second)
                if (bounds_[id].overlaps(area))
                    result.push_back(id);
        }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

} // namespace kinetra
