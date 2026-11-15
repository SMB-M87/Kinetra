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
    if (!finite(p) || p.x < 0 || p.y < 0 || p.x >= width_ * cell_size_.x || p.y >= height_ * cell_size_.y)
        return -1;
    return int(p.y / cell_size_.y) * width_ + int(p.x / cell_size_.x);
}
Vec2 WeightedGrid::center(int id) const {
    blocked_.at(id);
    return {(id % width_ + 0.5f) * cell_size_.x, (id / width_ + 0.5f) * cell_size_.y};
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
std::vector<Vec2> WeightedGrid::path(Vec2 start, Vec2 goal) const {
    const int first = cell(start), last = cell(goal);
    if (blocked(first) || blocked(last))
        return {};
    auto heuristic = [&](int id) { return double(distance(center(id), center(last))); };
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
                const double step = distance(center(current), center(next));
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
            const Vec2 a{x * cell_size_.x, y * cell_size_.y}, b = a + cell_size_;
            if (x == 0 || blocked(id - 1))
                result.push_back({a, {a.x, b.y}});
            if (x + 1 == width_ || blocked(id + 1))
                result.push_back({{b.x, a.y}, b});
            if (y == 0 || blocked(id - width_))
                result.push_back({a, {b.x, a.y}});
            if (y + 1 == height_ || blocked(id + width_))
                result.push_back({{a.x, b.y}, b});
        }
    return result;
}
} // namespace kinetra
