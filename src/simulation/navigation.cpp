#include <algorithm>
#include <functional>
#include <kinetra/simulation/navigation.h>
#include <limits>
#include <queue>
#include <stdexcept>

namespace kinetra {

Navigation::Navigation(float resolution, float radius) : spacing(resolution), clearance(radius) {
    if (!std::isfinite(resolution) || resolution <= 0 || !std::isfinite(radius) || radius <= 0)
        throw std::invalid_argument("Navigation resolution and radius must be finite and positive");
}

void Navigation::rebuild(kinetra::Aabb bounds, const std::vector<kinetra::Body> &obstacles) {
    if (!kinetra::finite(bounds.min) || !kinetra::finite(bounds.max) || bounds.min.x > bounds.max.x ||
        bounds.min.y > bounds.max.y || (double(bounds.max.x) - bounds.min.x) / spacing > 262142 ||
        (double(bounds.max.y) - bounds.min.y) / spacing > 262142)
        throw std::invalid_argument("Invalid or oversized navigation bounds");
    const int new_width = int(std::floor((bounds.max.x - bounds.min.x) / spacing)) + 1;
    const int new_height = int(std::floor((bounds.max.y - bounds.min.y) / spacing)) + 1;
    if (std::int64_t(new_width) * new_height > 262144)
        throw std::invalid_argument("Navigation view exceeds 262144 cells; zoom in");
    area = bounds;
    width = new_width;
    height = new_height;
    geometry = obstacles;
    std::vector<kinetra::Aabb> boxes;
    for (const auto &body : geometry) {
        kinetra::validate(body);
        boxes.push_back(kinetra::bounds(body));
    }
    index.rebuild(boxes);
    blocked.assign(width * height, false);
    for (int id = 0; id < int(blocked.size()); ++id)
        blocked[id] = !clear(point(id), point(id));
    // Cache swept neighbor links and connected regions once per terrain change.
    // Occupied clicks can then choose the nearest reachable node without a
    // full Dijkstra traversal for every member of a commanded crowd.
    links.assign(blocked.size(), 0);
    for (int id = 0; id < int(blocked.size()); ++id) {
        if (blocked[id])
            continue;
        for (int direction = 0; direction < 8; ++direction) {
            const int x = id % width + offsets[direction].first;
            const int y = id / width + offsets[direction].second;
            if (x < 0 || y < 0 || x >= width || y >= height)
                continue;
            const int next = y * width + x;
            if (next > id && !blocked[next] && clear(point(id), point(next))) {
                links[id] |= 1 << direction;
                links[next] |= 1 << (7 - direction);
            }
        }
    }
    region.assign(blocked.size(), -1);
    std::vector<int> pending;
    for (int id = 0; id < int(blocked.size()); ++id) {
        if (blocked[id] || region[id] >= 0)
            continue;
        region[id] = id;
        pending.push_back(id);
        while (!pending.empty()) {
            const int current = pending.back();
            pending.pop_back();
            for (int direction = 0; direction < 8; ++direction) {
                if (!(links[current] & (1 << direction)))
                    continue;
                const int next = neighbor(current, direction);
                if (region[next] < 0) {
                    region[next] = id;
                    pending.push_back(next);
                }
            }
        }
    }
}

std::vector<kinetra::Vec2> Navigation::path(kinetra::Vec2 start, kinetra::Vec2 goal) const {
    if (!inside(start) || !inside(goal))
        return {};
    if (!clear(start, start)) {
        // Contacts and manual movement can enter the conservative planning
        // envelope. Depart monotonically from each nearby surface before A*.
        std::vector<std::pair<float, kinetra::Vec2>> candidates;
        for (int direction = 0; direction < 16; ++direction) {
            const auto escape =
                start + kinetra::rotated({2, 0}, static_cast<float>(direction) * 0.392699082f);
            if (inside(escape) && clear(escape, escape) && can_depart(start, escape))
                candidates.emplace_back(kinetra::length(escape - goal), escape);
        }
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto &a, const auto &b) { return a.first < b.first; });
        for (const auto &[distance, escape] : candidates) {
            auto route = path(escape, goal);
            if (!route.empty()) {
                route.insert(route.begin(), escape);
                return route;
            }
        }
        return {};
    }
    int source = -1;
    float nearest = std::numeric_limits<float>::max();
    // Connect the actual pose, rather than snapping through an obstacle to a cell.
    const int cx = int(std::round((start.x - area.min.x) / spacing));
    const int cy = int(std::round((start.y - area.min.y) / spacing));
    for (int y = std::max(0, cy - 2); y <= std::min(height - 1, cy + 2); ++y)
        for (int x = std::max(0, cx - 2); x <= std::min(width - 1, cx + 2); ++x) {
            const int id = y * width + x;
            const float distance = kinetra::length(point(id) - start);
            if (!blocked[id] && distance < nearest && clear(start, point(id))) {
                source = id;
                nearest = distance;
            }
        }
    if (source < 0)
        return {};
    if (!clear(goal, goal)) {
        float closest = std::numeric_limits<float>::max();
        int destination = -1;
        for (int id = 0; id < int(blocked.size()); ++id) {
            if (region[id] != region[source])
                continue;
            const float distance = kinetra::length(point(id) - goal);
            if (distance < closest) {
                closest = distance;
                destination = id;
            }
        }
        goal = point(destination);
    }
    if (clear(start, goal))
        return {goal};
    using Entry = std::pair<float, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    std::vector<float> cost(blocked.size(), std::numeric_limits<float>::max());
    std::vector<int> parent(blocked.size(), -1);
    const auto heuristic = [&](int id) { return kinetra::length(point(id) - goal); };
    cost[source] = 0;
    open.push({heuristic(source), source});
    int destination = -1;
    while (!open.empty()) {
        const auto [score, current] = open.top();
        open.pop();
        if (score > cost[current] + heuristic(current) + 0.0001f)
            continue;
        const float distance = kinetra::length(point(current) - goal);
        if (distance <= spacing * 2 && clear(point(current), goal)) {
            destination = current;
            break;
        }
        for (int direction = 0; direction < 8; ++direction) {
            if (!(links[current] & (1 << direction)))
                continue;
            const int next = neighbor(current, direction);
            const auto [dx, dy] = offsets[direction];
            const float next_cost = cost[current] + (dx && dy ? 1.41421356f : 1.f) * spacing;
            if (next_cost >= cost[next])
                continue;
            cost[next] = next_cost;
            parent[next] = current;
            open.push({next_cost + heuristic(next), next});
        }
    }
    if (destination < 0)
        return {};
    std::vector<kinetra::Vec2> raw{goal};
    for (int id = destination; id >= 0; id = parent[id])
        raw.push_back(point(id));
    raw.push_back(start);
    std::reverse(raw.begin(), raw.end());
    // Greedy string pulling removes grid stairs; every shortcut is swept against
    // the original circle/rotated-box geometry, including diagonal corner cuts.
    std::vector<kinetra::Vec2> route;
    for (std::size_t from = 0; from + 1 < raw.size();) {
        std::size_t to = raw.size() - 1;
        while (to > from + 1 && !clear(raw[from], raw[to]))
            --to;
        route.push_back(raw[to]);
        from = to;
    }
    return route;
}

bool Navigation::clear(kinetra::Vec2 a, kinetra::Vec2 b) const {
    for (auto id : index.query({{std::min(a.x, b.x) - clearance, std::min(a.y, b.y) - clearance},
                                {std::max(a.x, b.x) + clearance, std::max(a.y, b.y) + clearance}})) {
        if (!clear_body(geometry[id], a, b))
            return false;
    }
    return true;
}

bool Navigation::clear_body(const kinetra::Body &body, kinetra::Vec2 a, kinetra::Vec2 b) const {
    if (body.shape == kinetra::Shape::circle)
        return distance_to_segment(body.position, a, b) > body.radius + clearance;
    const auto p = kinetra::rotated(a - body.position, -body.angle);
    const auto q = kinetra::rotated(b - body.position, -body.angle);
    const auto e = kinetra::local_extents(body);
    if ((std::abs(p.x) <= e.x && std::abs(p.y) <= e.y) || (std::abs(q.x) <= e.x && std::abs(q.y) <= e.y))
        return false;
    const kinetra::Vec2 corners[]{{-e.x, -e.y}, {e.x, -e.y}, {e.x, e.y}, {-e.x, e.y}};
    for (int i = 0; i < 4; ++i) {
        const auto u = corners[i], v = corners[(i + 1) % 4];
        const auto d = q - p, edge = v - u;
        const float denominator = kinetra::cross(d, edge);
        if (std::abs(denominator) > 0.000001f) {
            const float t = kinetra::cross(u - p, edge) / denominator;
            const float s = kinetra::cross(u - p, d) / denominator;
            if (t >= 0 && t <= 1 && s >= 0 && s <= 1)
                return false;
        }
        if (std::min({distance_to_segment(p, u, v), distance_to_segment(q, u, v),
                      distance_to_segment(u, p, q), distance_to_segment(v, p, q)}) <= clearance)
            return false;
    }
    return true;
}

bool Navigation::can_depart(kinetra::Vec2 start, kinetra::Vec2 end) const {
    for (auto id :
         index.query({{std::min(start.x, end.x) - clearance, std::min(start.y, end.y) - clearance},
                      {std::max(start.x, end.x) + clearance, std::max(start.y, end.y) + clearance}})) {
        const auto &body = geometry[id];
        auto a = kinetra::rotated(start - body.position, -body.angle);
        auto b = kinetra::rotated(end - body.position, -body.angle);
        kinetra::Vec2 outward;
        float gap;
        if (body.shape == kinetra::Shape::circle) {
            outward = a;
            gap = kinetra::length(a) - body.radius;
        } else {
            const auto e = kinetra::local_extents(body);
            outward = a - kinetra::Vec2{std::clamp(a.x, -e.x, e.x), std::clamp(a.y, -e.y, e.y)};
            gap = kinetra::length(outward);
        }
        if (gap <= clearance) {
            if (gap <= 0 || kinetra::dot(outward, b - a) <= 0)
                return false;
        } else {
            // Reuse the exact sweep for unrelated obstacles.
            if (!clear_body(body, start, end))
                return false;
        }
    }
    return true;
}

int Navigation::neighbor(int id, int direction) const {
    return id + offsets[direction].first + offsets[direction].second * width;
}

bool Navigation::inside(kinetra::Vec2 p) const {
    return kinetra::finite(p) && !blocked.empty() && p.x >= area.min.x && p.y >= area.min.y &&
           p.x <= area.max.x && p.y <= area.max.y;
}

kinetra::Vec2 Navigation::point(int id) const {
    return area.min + kinetra::Vec2{float(id % width) * spacing, float(id / width) * spacing};
}

kinetra::Vec2 route_target(kinetra::Vec2 position, std::vector<kinetra::Vec2> &route,
                           const Navigation &navigation) {
    while (route.size() > 1 && navigation.clear(position, route[1]))
        route.erase(route.begin());
    if (route.empty())
        return position;
    if (route.size() == 1)
        return route.front();
    const auto corner = route.front();
    const float distance = kinetra::length(corner - position);
    if (distance < 2) {
        const auto ahead = corner + kinetra::limited(route[1] - corner, 2 - distance);
        if (navigation.clear(position, ahead))
            return ahead;
    }
    return corner;
}

} // namespace kinetra
