#include <array>
#include <kinetra/simulation/avoidance.h>
#include <kinetra/simulation/physics.h>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace kinetra {
namespace {
struct Plane {
    Vec2 normal;
    double minimum;
};
struct Disc {
    Vec2 center;
    double radius;
};
double dot_precise(Vec2 a, Vec2 b) {
    return double(a.x) * b.x + double(a.y) * b.y;
}
constexpr double epsilon = 0.000001;

void validate_agent(const AvoidanceAgent &agent) {
    if (!finite(agent.position) || !finite(agent.velocity) || !std::isfinite(length(agent.velocity)) ||
        !std::isfinite(agent.radius) || agent.radius <= 0)
        throw std::invalid_argument("Avoidance agents require finite state and a positive radius");
    const auto half = agent.half_extents;
    if (!finite(half) ||
        !((half.x == 0 && half.y == 0) ||
          (half.x > 0 && half.y > 0 && std::isfinite(length(half)) && agent.radius >= length(half))))
        throw std::invalid_argument("Avoidance rectangles require positive extents and an enclosing radius");
}

bool inside(Vec2 point, const Disc &disc) {
    const auto offset = point - disc.center;
    if (disc.radius == 0)
        return offset.x == 0 && offset.y == 0;
    const double radius = disc.radius + epsilon;
    return dot_precise(offset, offset) <= radius * radius;
}

// The nearest point in a two-disc intersection is either a valid radial projection
// or a circle-circle intersection. This also handles zero acceleration and overspeed.
Vec2 nearest_in_discs(Vec2 preferred, const Disc &speed, const Disc &reachable) {
    std::optional<Vec2> best;
    double cost = std::numeric_limits<double>::max();
    auto consider = [&](Vec2 point) {
        if (!inside(point, speed) || !inside(point, reachable))
            return;
        const auto delta = point - preferred;
        const double distance = dot_precise(delta, delta);
        if (distance < cost) {
            cost = distance;
            best = point;
        }
    };
    consider(preferred);
    consider(speed.center + limited(preferred - speed.center, float(speed.radius)));
    consider(reachable.center + limited(preferred - reachable.center, float(reachable.radius)));
    const Vec2 offset = reachable.center - speed.center;
    const double distance = length(offset);
    if (distance > 0 && distance <= speed.radius + reachable.radius + epsilon &&
        distance + std::min(speed.radius, reachable.radius) + epsilon >=
            std::max(speed.radius, reachable.radius)) {
        const double along =
            (speed.radius * speed.radius - reachable.radius * reachable.radius + distance * distance) /
            (2 * distance);
        const double height = std::sqrt(std::max(0.0, speed.radius * speed.radius - along * along));
        const Vec2 direction = offset / float(distance);
        const Vec2 base = speed.center + direction * float(along);
        consider(base + perpendicular(direction) * float(height));
        consider(base - perpendicular(direction) * float(height));
    }
    // The caller expands the speed disc to include current velocity, so it is
    // always a valid fallback despite roundoff in nearly tangent intersections.
    return best.value_or(reachable.center);
}

std::optional<Vec2> solve(std::span<const Plane> planes, Vec2 preferred, const Disc &speed,
                          const Disc &reachable, double relaxation) {
    Vec2 result = nearest_in_discs(preferred, speed, reachable);
    for (std::size_t i = 0; i < planes.size(); ++i) {
        const auto &plane = planes[i];
        const double boundary = plane.minimum - relaxation;
        if (dot_precise(plane.normal, result) >= boundary - epsilon)
            continue;
        const Vec2 origin = plane.normal * float(boundary);
        const Vec2 tangent = perpendicular(plane.normal);
        double lower = -std::numeric_limits<double>::max(), upper = -lower;
        for (const auto &disc : {speed, reachable}) {
            const Vec2 delta = origin - disc.center;
            const double perpendicular_distance = dot_precise(delta, plane.normal);
            const double square = disc.radius * disc.radius - perpendicular_distance * perpendicular_distance;
            if (square < -epsilon)
                return {};
            const double middle = -dot_precise(delta, tangent);
            const double half = std::sqrt(std::max(0.0, square));
            lower = std::max(lower, middle - half);
            upper = std::min(upper, middle + half);
        }
        for (std::size_t j = 0; j < i; ++j) {
            const double rate = dot_precise(planes[j].normal, tangent);
            const double required = planes[j].minimum - relaxation - dot_precise(planes[j].normal, origin);
            if (std::abs(rate) < 0.00000001) {
                if (required > epsilon)
                    return {};
            } else if (rate > 0) {
                lower = std::max(lower, required / rate);
            } else {
                upper = std::min(upper, required / rate);
            }
        }
        if (lower > upper + epsilon)
            return {};
        const double t = std::clamp(dot_precise(preferred - origin, tangent), lower, std::max(lower, upper));
        result = origin + tangent * float(t);
    }
    return result;
}

// For fixed axis-aligned rectangles the Minkowski obstacle is another rectangle.
// Its finite-horizon velocity obstacle consists of origin-facing cutoff edges
// and the two tangent rays. Project onto that boundary, never onto internal rays.
Plane rectangle_constraint(const AvoidanceAgent &a, const AvoidanceAgent &b, float dt,
                           const AvoidanceSettings &settings) {
    const Vec2 position = b.position - a.position;
    const Vec2 relative = a.velocity - b.velocity;
    const Vec2 extent = a.half_extents + b.half_extents + Vec2{settings.clearance, settings.clearance};
    const bool overlapping = std::abs(position.x) <= extent.x && std::abs(position.y) <= extent.y;
    const float horizon = overlapping ? dt : settings.horizon;
    const Vec2 low = (position - extent) / horizon, high = (position + extent) / horizon;
    if (!finite(low) || !finite(high))
        throw std::invalid_argument("Avoidance rectangle exceeds numerical range");
    const std::array<Vec2, 4> corners{{low, {high.x, low.y}, high, {low.x, high.y}}};
    const std::array<Vec2, 4> outward{{{0, -1}, {1, 0}, {0, 1}, {-1, 0}}};
    std::array<bool, 4> visible{};
    for (int i = 0; i < 4; ++i)
        visible[i] = dot_precise(outward[i], corners[i]) < 0;
    // Slab test of the relative travel segment against the Minkowski box.
    double enter = 0, leave = 1;
    for (int axis = 0; axis < 2; ++axis) {
        const double velocity = axis == 0 ? relative.x : relative.y;
        const double lo = axis == 0 ? low.x : low.y, hi = axis == 0 ? high.x : high.y;
        if (velocity == 0) {
            if (lo > 0 || hi < 0)
                leave = -1;
        } else {
            enter = std::max(enter, std::min(lo / velocity, hi / velocity));
            leave = std::min(leave, std::max(lo / velocity, hi / velocity));
        }
    }
    const bool inside = overlapping ? relative.x >= low.x && relative.x <= high.x && relative.y >= low.y &&
                                          relative.y <= high.y
                                    : enter <= leave;
    Vec2 boundary{}, normal{};
    double best = std::numeric_limits<double>::max();
    // Equal-distance escape faces must choose opposite sides for reciprocal
    // calls, including coincident centers and thin boxes overlapping sideways.
    const Vec2 tie_direction = (length(position) > 0 ? -normalized(position) : Vec2{}) +
                               (a.id < b.id ? Vec2{-1, -0.5f} : Vec2{1, 0.5f}) * 0.0001f;
    auto consider = [&](Vec2 point, Vec2 outward_normal) {
        const Vec2 delta = point - relative;
        const double cost = dot_precise(delta, delta);
        const Vec2 candidate =
            cost > epsilon * epsilon ? normalized(delta) * (inside ? 1.f : -1.f) : outward_normal;
        if (cost < best - epsilon * epsilon || (std::abs(cost - best) <= epsilon * epsilon &&
                                                dot(candidate, tie_direction) > dot(normal, tie_direction))) {
            best = cost;
            boundary = point;
            normal = candidate;
        }
    };
    for (int i = 0; i < 4; ++i) {
        if (overlapping || visible[i]) {
            const Vec2 edge = corners[(i + 1) % 4] - corners[i];
            const double square = dot_precise(edge, edge);
            if (square <= 0)
                throw std::invalid_argument("Avoidance rectangle collapsed numerically");
            const float along =
                float(std::clamp(dot_precise(relative - corners[i], edge) / square, 0.0, 1.0));
            consider(corners[i] + edge * along, outward[i]);
        }
        if (!overlapping && visible[i] != visible[(i + 3) % 4]) {
            const Vec2 ray = normalized(corners[i]);
            Vec2 ray_normal = perpendicular(ray);
            if (dot(ray_normal, position) > 0)
                ray_normal = -ray_normal;
            consider(corners[i] + ray * std::max(0.f, dot(relative - corners[i], ray)), ray_normal);
        }
    }
    const Vec2 shared = a.velocity + (boundary - relative) * (b.cooperative ? 0.5f : 1.f);
    if (!finite(normal) || length(normal) < 0.9f || !finite(shared))
        throw std::invalid_argument("Avoidance rectangle constraint exceeds numerical range");
    return {normal, dot_precise(normal, shared)};
}

// Construct the nearest boundary of the truncated relative-velocity obstacle:
// its cutoff circle or one of the two tangent rays. The separating plane shares
// the required correction between cooperating agents (Van den Berg et al., ORCA).
Plane constraint(const AvoidanceAgent &a, const AvoidanceAgent &b, float dt,
                 const AvoidanceSettings &settings) {
    if (a.half_extents.x > 0 && b.half_extents.x > 0)
        return rectangle_constraint(a, b, dt, settings);
    const Vec2 position = b.position - a.position;
    const Vec2 relative = a.velocity - b.velocity;
    const float radius = a.radius + b.radius + settings.clearance;
    const double distance_squared = dot_precise(position, position);
    const bool overlapping = distance_squared <= double(radius) * radius;
    const float horizon = overlapping ? dt : settings.horizon;
    const Vec2 offset = relative - position / horizon;
    const double projection = dot_precise(offset, position);
    Vec2 normal, correction;
    if (overlapping ||
        (projection < 0 && projection * projection > double(radius) * radius * dot_precise(offset, offset))) {
        normal = length(offset) > 0 ? normalized(offset) : (a.id < b.id ? Vec2{-1, 0} : Vec2{1, 0});
        correction = normal * (radius / horizon - length(offset));
    } else {
        const Vec2 forward = normalized(position);
        const float sine = radius / float(std::sqrt(distance_squared));
        const float cosine = std::sqrt(std::max(0.f, 1 - sine * sine));
        const bool left = cross(position, relative) > 0;
        const Vec2 ray = forward * cosine + perpendicular(forward) * (left ? sine : -sine);
        normal = perpendicular(ray) * (left ? 1.f : -1.f);
        correction = ray * dot(relative, ray) - relative;
    }
    const Vec2 boundary = a.velocity + correction * (b.cooperative ? 0.5f : 1.f);
    if (!finite(normal) || length(normal) < 0.9f || !finite(boundary))
        throw std::invalid_argument("Avoidance geometry exceeds numerical range");
    return {normal, dot_precise(normal, boundary)};
}
} // namespace

AvoidanceResult avoid_collisions(const AvoidanceAgent &agent, Vec2 preferred,
                                 std::span<const AvoidanceAgent> neighbors, float dt,
                                 const AvoidanceSettings &settings) {
    validate_agent(agent);
    if (!finite(preferred) || !std::isfinite(length(preferred)) || !std::isfinite(dt) || dt <= 0 ||
        !std::isfinite(settings.horizon) || settings.horizon <= 0)
        throw std::invalid_argument("Avoidance requires finite preferred velocity and positive dt/horizon");
    for (float value :
         {settings.max_speed, settings.max_acceleration, settings.clearance, settings.passing_bias})
        if (!std::isfinite(value) || value < 0)
            throw std::invalid_argument("Avoidance limits must be finite and nonnegative");
    if (!std::isfinite(settings.max_speed * settings.max_speed) ||
        !std::isfinite(settings.max_acceleration * settings.max_acceleration))
        throw std::invalid_argument("Avoidance motion limits exceed numerical range");
    std::vector<const AvoidanceAgent *> ordered;
    ordered.reserve(neighbors.size());
    bool conflict = false;
    for (const auto &neighbor : neighbors) {
        validate_agent(neighbor);
        if (neighbor.id == agent.id)
            throw std::invalid_argument("Avoidance neighbors must exclude the agent");
        ordered.push_back(&neighbor);
        // A passing convention must start before a perfectly symmetric crowd
        // reaches a packed standstill, where local velocity planes can lock it.
        const Vec2 offset = neighbor.position - agent.position;
        const double range = double(agent.radius) + neighbor.radius + settings.clearance +
                             (settings.max_speed + std::max(settings.max_speed, length(neighbor.velocity))) *
                                 double(settings.horizon);
        conflict =
            conflict || (dot_precise(offset, preferred) > 0 && dot_precise(offset, offset) < range * range);
        conflict = conflict || segment_circle({}, (preferred - neighbor.velocity) * settings.horizon,
                                              neighbor.position - agent.position,
                                              agent.radius + neighbor.radius + settings.clearance)
                                   .has_value();
    }
    std::sort(ordered.begin(), ordered.end(), [](auto a, auto b) { return a->id < b->id; });
    std::vector<Plane> planes;
    planes.reserve(ordered.size());
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        if (i && ordered[i]->id == ordered[i - 1]->id)
            throw std::invalid_argument("Avoidance neighbor IDs must be unique");
        planes.push_back(constraint(agent, *ordered[i], dt, settings));
    }
    if (conflict)
        preferred -= perpendicular(normalized(preferred)) *
                     (settings.passing_bias * std::min(settings.max_speed, length(preferred)));
    if (!finite(preferred) || !std::isfinite(length(preferred)))
        throw std::invalid_argument("Avoidance preference exceeds numerical range");
    const Disc speed{{}, std::max(settings.max_speed, length(agent.velocity))};
    const Disc reachable{agent.velocity, double(settings.max_acceleration) * dt};
    auto solution = solve(planes, preferred, speed, reachable, 0);
    if (!solution) {
        // Minimize the largest half-plane violation without relaxing physical limits.
        double lower = 0, upper = 0;
        for (const auto &plane : planes)
            upper = std::max(upper, plane.minimum - dot_precise(plane.normal, agent.velocity));
        upper += epsilon;
        solution = agent.velocity;
        for (int iteration = 0; iteration < 20; ++iteration) {
            const double middle = (lower + upper) / 2;
            if (auto candidate = solve(planes, preferred, speed, reachable, middle)) {
                upper = middle;
                solution = candidate;
            } else {
                lower = middle;
            }
        }
    }
    AvoidanceResult result;
    const double ax = (double(solution->x) - agent.velocity.x) / dt;
    const double ay = (double(solution->y) - agent.velocity.y) / dt;
    const double magnitude = std::hypot(ax, ay);
    const double scale = magnitude > settings.max_acceleration ? settings.max_acceleration / magnitude : 1;
    result.acceleration = {float(ax * scale), float(ay * scale)};
    result.velocity = agent.velocity + result.acceleration * dt;
    if (!finite(result.velocity) || !finite(result.acceleration))
        throw std::invalid_argument("Avoidance result exceeds numerical range");
    for (const auto &plane : planes)
        result.violation =
            std::max(result.violation, float(plane.minimum - dot_precise(plane.normal, result.velocity)));
    result.feasible = result.violation <= 0.00001f;
    return result;
}
} // namespace kinetra
