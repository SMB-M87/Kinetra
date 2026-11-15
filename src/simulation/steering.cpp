#include <limits>
#include <kinetra/steering.h>
#include <stdexcept>

namespace kinetra {
namespace {

/// Ray versus expanded box. Return first entry time and outward normal; slab
/// intersection also handles parallel rays and starts inside an obstacle.
bool ray_box(Vec2 origin, Vec2 velocity, Vec2 extent, float horizon, float &hit, Vec2 &normal) {
    float enter = 0, exit = horizon;
    Vec2 entry_normal{};
    for (int axis = 0; axis < 2; ++axis) {
        const float p = axis == 0 ? origin.x : origin.y;
        const float v = axis == 0 ? velocity.x : velocity.y;
        const float e = axis == 0 ? extent.x : extent.y;
        if (std::abs(v) < 0.000001f) {
            if (std::abs(p) > e)
                return false;
            continue;
        }
        float near = (-e - p) / v, far = (e - p) / v;
        Vec2 n = axis == 0 ? Vec2{-1, 0} : Vec2{0, -1};
        if (near > far) {
            std::swap(near, far);
            n = -n;
        }
        if (near > enter) {
            enter = near;
            entry_normal = n;
        }
        exit = std::min(exit, far);
        if (enter > exit)
            return false;
    }
    if (length(entry_normal) == 0) {
        const float x = extent.x - std::abs(origin.x), y = extent.y - std::abs(origin.y);
        entry_normal = x < y ? Vec2{origin.x >= 0 ? 1.f : -1.f, 0} : Vec2{0, origin.y >= 0 ? 1.f : -1.f};
    }
    hit = enter;
    normal = entry_normal;
    return exit >= 0;
}

} // namespace

Vec2 steer(const Body &a, Vec2 target, std::span<const Body> obstacles, const Steering &s) {
    validate(a);
    for (float v : {s.max_speed, s.max_acceleration, s.response_time, s.slow_radius, s.stop_radius,
                    s.lookahead, s.clearance, s.separation_radius, s.separation_weight})
        if (!std::isfinite(v) || v < 0)
            throw std::invalid_argument("Steering settings must be finite and nonnegative");
    if (!finite(target) || s.response_time == 0 || s.slow_radius <= s.stop_radius)
        throw std::invalid_argument("Invalid steering target, response time or arrival radii");
    const Vec2 offset = target - a.position;
    const float distance = length(offset);
    const float speed =
        s.max_speed * std::clamp((distance - s.stop_radius) / (s.slow_radius - s.stop_radius), 0.f, 1.f);
    const Vec2 desired = normalized(offset) * speed;
    Vec2 acceleration = limited((desired - a.velocity) / s.response_time, s.max_acceleration);
    Vec2 separation{};
    for (const auto &neighbor : obstacles) {
        validate(neighbor);
        const Vec2 away = a.position - neighbor.position;
        const float distance = length(away);
        if (distance > 0 && distance < s.separation_radius)
            separation += normalized(away) * (1 - distance / s.separation_radius);
    }
    acceleration =
        limited(acceleration + separation * (s.max_acceleration * s.separation_weight), s.max_acceleration);
    const Vec2 travel = length(a.velocity) > 0.2f ? a.velocity : desired;
    if (length(travel) < 0.00001f)
        return acceleration;
    const float agent_radius = a.shape == Shape::circle ? a.radius : length(a.half_extents);
    float nearest = std::numeric_limits<float>::max();
    Vec2 avoidance_normal{};
    for (const Body &obstacle : obstacles) {
        validate(obstacle);
        const float radius = agent_radius + s.clearance;
        const Vec2 extent = obstacle.shape == Shape::circle
                                ? Vec2{obstacle.radius + radius, obstacle.radius + radius}
                                : obstacle.half_extents + Vec2{radius, radius};
        float hit;
        Vec2 normal;
        const Vec2 relative_velocity =
            travel - (obstacle.motion == Motion::fixed ? Vec2{} : obstacle.velocity);
        if (ray_box(rotated(a.position - obstacle.position, -obstacle.angle),
                    rotated(relative_velocity, -obstacle.angle), extent, s.lookahead, hit, normal) &&
            hit < nearest) {
            nearest = hit;
            avoidance_normal = rotated(normal, obstacle.angle);
        }
    }
    if (nearest < std::numeric_limits<float>::max()) {
        Vec2 tangent = perpendicular(avoidance_normal);
        // Keep a consistent passing side relative to the goal. Choosing the side
        // closest to the goal flips every time a centered agent crosses its ray.
        if (dot(tangent, perpendicular(offset)) < 0)
            tangent = -tangent;
        // Give avoidance priority over arrival so a direct goal cannot cancel it.
        const Vec2 avoid_velocity = normalized(avoidance_normal * 0.8f + tangent) * s.max_speed;
        acceleration = limited((avoid_velocity - a.velocity) / s.response_time, s.max_acceleration);
    }
    return acceleration;
}

} // namespace kinetra
