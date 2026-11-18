#include <kinetra/weighted_grid.h>
#include <limits>
#include <queue>
#include <stdexcept>

namespace kinetra {
WeightedGrid::WeightedGrid(Vec2 dimension, Vec2 size) : cell_size_(size) {
    if (!finite(dimension) || !finite(size) || size.x <= 0 || size.y <= 0 || dimension.x < size.x ||
        dimension.y < size.y || dimension.x / size.x > 100000 || dimension.y / size.y > 100000)
        throw std::invalid_argument("Invalid weighted grid dimensions");
    width_ = int(std::floor(dimension.x / size.x));
    height_ = int(std::floor(dimension.y / size.y));
    if (static_cast<long long>(width_) * height_ > 1000000)
        throw std::invalid_argument("Weighted grid exceeds one million cells");
    blocked_.resize(width_ * height_);
    weights_.resize(blocked_.size());
}
int WeightedGrid::cell(Vec2 p) const {
    if (!finite(p) || p.x < 0 || p.y < 0 || p.x >= static_cast<float>(width_) * cell_size_.x ||
        p.y >= static_cast<float>(height_) * cell_size_.y)
        return -1;
    return int(p.y / cell_size_.y) * width_ + int(p.x / cell_size_.x);
}
Vec2 WeightedGrid::center(int id) const {
    static_cast<void>(blocked_.at(id));
    return {(static_cast<float>(id % width_) + 0.5f) * cell_size_.x,
            (static_cast<float>(id / width_) + 0.5f) * cell_size_.y};
}
bool WeightedGrid::blocked(int id) const {
    return id < 0 || id >= int(blocked_.size()) || blocked_[id];
}
void WeightedGrid::block(int id, bool value) {
    blocked_.at(id) = value;
}
float WeightedGrid::weight(int id) const {
    return weights_.at(id);
}
void WeightedGrid::weights(std::span<const float> values) {
    if (values.size() != weights_.size())
        throw std::invalid_argument("Grid weight count mismatch");
    for (float value : values)
        if (!std::isfinite(value) || value < 0)
            throw std::invalid_argument("Grid costs must be finite and nonnegative");
    weights_.assign(values.begin(), values.end());
}
bool WeightedGrid::clear(Vec2 start, Vec2 goal, Vec2 extent) const {
    if (!finite(start) || !finite(goal) || !finite(extent) || extent.x < 0 || extent.y < 0)
        throw std::invalid_argument("Grid clearance requires finite points and nonnegative half extents");
    const Vec2 low{std::min(start.x, goal.x) - extent.x, std::min(start.y, goal.y) - extent.y};
    const Vec2 high{std::max(start.x, goal.x) + extent.x, std::max(start.y, goal.y) + extent.y};
    if (low.x < 0 || low.y < 0 || high.x > static_cast<float>(width_) * cell_size_.x ||
        high.y > static_cast<float>(height_) * cell_size_.y || blocked(cell(start)) || blocked(cell(goal)))
        return false;
    const Vec2 travel = goal - start;
    for (int y = std::max(0, int(low.y / cell_size_.y));
         y <= std::min(height_ - 1, int(high.y / cell_size_.y)); ++y)
        for (int x = std::max(0, int(low.x / cell_size_.x));
             x <= std::min(width_ - 1, int(high.x / cell_size_.x)); ++x) {
            if (!blocked(y * width_ + x))
                continue;
            const Vec2 minimum =
                Vec2{static_cast<float>(x) * cell_size_.x, static_cast<float>(y) * cell_size_.y} - extent;
            const Vec2 maximum =
                Vec2{static_cast<float>(x + 1) * cell_size_.x, static_cast<float>(y + 1) * cell_size_.y} +
                extent;
            float enter = 0, exit = 1;
            bool intersects = true;
            for (int axis = 0; axis < 2; ++axis) {
                const float p = axis == 0 ? start.x : start.y;
                const float v = axis == 0 ? travel.x : travel.y;
                const float lo = axis == 0 ? minimum.x : minimum.y;
                const float hi = axis == 0 ? maximum.x : maximum.y;
                if (v == 0) {
                    if (p <= lo || p >= hi)
                        intersects = false;
                } else {
                    float near = (lo - p) / v, far = (hi - p) / v;
                    if (near > far)
                        std::swap(near, far);
                    enter = std::max(enter, near);
                    exit = std::min(exit, far);
                    if (enter >= exit)
                        intersects = false;
                }
            }
            if (intersects)
                return false;
        }
    return true;
}
std::vector<Vec2> WeightedGrid::path(Vec2 start, Vec2 goal, Vec2 extent) const {
    if (!finite(start) || !finite(goal))
        return {};
    const int first = cell(start), last = cell(goal);
    if (!clear(start, start, extent) || !clear(goal, goal, extent))
        return {};
    if (first == last)
        return clear(start, goal, extent) ? std::vector<Vec2>{goal} : std::vector<Vec2>{};
    auto point = [&](int id) { return id == first ? start : id == last ? goal : center(id); };
    auto heuristic = [&](int id) { return double(distance(point(id), goal)); };
    using Entry = std::pair<double, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    std::vector<double> cost(blocked_.size(), std::numeric_limits<double>::infinity());
    std::vector<int> previous(blocked_.size(), -1);
    std::vector<bool> closed(blocked_.size());
    cost[first] = 0;
    open.emplace(heuristic(first), first);
    while (!open.empty()) {
        const int current = open.top().second;
        open.pop();
        if (closed[current])
            continue;
        if (current == last) {
            std::vector<Vec2> route{goal};
            for (int id = last; id != first;) {
                id = previous[id];
                if (id != first)
                    route.push_back(center(id));
            }
            std::reverse(route.begin(), route.end());
            return route;
        }
        closed[current] = true;
        const int x = current % width_, y = current / width_;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if ((!dx && !dy) || x + dx < 0 || x + dx >= width_ || y + dy < 0 || y + dy >= height_)
                    continue;
                const int next = (y + dy) * width_ + x + dx;
                if (blocked(next) || (dx && dy && (blocked(current + dx) || blocked(current + dy * width_))))
                    continue;
                if (!clear(point(current), point(next), extent))
                    continue;
                const double step = distance(point(current), point(next));
                const double candidate = cost[current] + step * (1.0 + weights_[next]);
                if (candidate < cost[next]) {
                    cost[next] = candidate;
                    previous[next] = current;
                    open.emplace(candidate + heuristic(next), next);
                }
            }
    }
    return {};
}
std::vector<Segment> WeightedGrid::borders() const {
    std::vector<Segment> result;
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x) {
            const int id = y * width_ + x;
            if (blocked(id))
                continue;
            const Vec2 a{static_cast<float>(x) * cell_size_.x, static_cast<float>(y) * cell_size_.y},
                b = a + cell_size_;
            if (x == 0 || blocked(id - 1))
                result.push_back({a, {a.x, b.y}});
            if (x + 1 == width_ || blocked(id + 1))
                result.push_back({{b.x, a.y}, b});
            if (y == 0 || blocked(id - width_))
                result.push_back({a, {b.x, a.y}});
            if (y + 1 == height_ || blocked(id + width_))
                result.push_back({{a.x, b.y}, b});
        }
    // Raster seams are not physical corners. Merge collinear touching segments so
    // end-cap steering and contact correction see only actual wall endpoints.
    auto vertical = [](const Segment &s) { return s.start.x == s.end.x; };
    std::sort(result.begin(), result.end(), [&](const Segment &a, const Segment &b) {
        if (vertical(a) != vertical(b))
            return vertical(a) < vertical(b);
        const float line_a = vertical(a) ? a.start.x : a.start.y;
        const float line_b = vertical(b) ? b.start.x : b.start.y;
        if (line_a != line_b)
            return line_a < line_b;
        return (vertical(a) ? a.start.y : a.start.x) < (vertical(b) ? b.start.y : b.start.x);
    });
    std::vector<Segment> merged;
    for (const auto &edge : result) {
        if (!merged.empty()) {
            auto &last = merged.back();
            if (vertical(last) == vertical(edge) && distance(last.end, edge.start) == 0) {
                last.end = edge.end;
                continue;
            }
        }
        merged.push_back(edge);
    }
    return merged;
}
} // namespace kinetra
