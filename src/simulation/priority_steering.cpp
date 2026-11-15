#include <kinetra/steering.h>
#include <stdexcept>

namespace kinetra {
SteeringForces priority_steer(const Body &a, Vec2 target, std::span<const Body> neighbors,
                              std::span<const Segment> borders, const PrioritySteering &s, bool arrival) {
    validate(a);
    for (float v : {s.max_speed, s.max_acceleration, s.arrival_radius, s.border_distance, s.border_strength,
                    s.border_lookahead, s.prediction_step, s.safety_multiplier})
        if (!std::isfinite(v) || v <= 0)
            throw std::invalid_argument("Priority steering settings must be finite and positive");
    if (a.shape != Shape::box || a.angle != 0 || !finite(target) || s.prediction_steps < 0 ||
        s.prediction_steps > 64)
        throw std::invalid_argument("Priority steering requires an axis-aligned box and bounded prediction");
    SteeringForces result;
    const Vec2 p = a.position + a.velocity * s.border_lookahead;
    auto repel = [&](Vec2 normal, float d) {
        if (d < s.border_distance) {
            const float strength =
                d <= 0 ? s.border_strength * 3
                       : s.border_strength * (1 / (d * d) - 1 / (s.border_distance * s.border_distance));
            result.border += normal * strength;
        }
    };
    for (const auto &edge : borders) {
        if (!finite(edge.start) || !finite(edge.end) ||
            (edge.start.x != edge.end.x && edge.start.y != edge.end.y))
            throw std::invalid_argument("Priority steering borders must be finite and axis-aligned");
        if (edge.start.x == edge.end.x && p.y >= std::min(edge.start.y, edge.end.y) &&
            p.y <= std::max(edge.start.y, edge.end.y)) {
            const float dx = p.x - edge.start.x;
            repel({dx >= 0 ? 1.f : -1.f, 0}, std::abs(dx) - a.half_extents.x);
        } else if (edge.start.y == edge.end.y && p.x >= std::min(edge.start.x, edge.end.x) &&
                   p.x <= std::max(edge.start.x, edge.end.x)) {
            const float dy = p.y - edge.start.y;
            repel({0, dy >= 0 ? 1.f : -1.f}, std::abs(dy) - a.half_extents.y);
        }
        for (Vec2 corner : {edge.start, edge.end}) {
            const Vec2 d = p - corner;
            repel(normalized(d), length({std::max(std::abs(d.x) - a.half_extents.x, 0.f),
                                         std::max(std::abs(d.y) - a.half_extents.y, 0.f)}));
        }
    }
    int predicted = 0;
    for (const auto &other : neighbors) {
        validate(other);
        if (other.shape != Shape::box || other.angle != 0)
            throw std::invalid_argument("Priority steering neighbors must be axis-aligned boxes");
        if (auto contact = collide(a, other); contact && contact->penetration > 0) {
            const Vec2 diff = a.position - other.position;
            result.collision +=
                length(diff) > 0.01f ? normalized(diff) * std::max(1.f, 10 / length(diff)) : -contact->normal;
        }
        if (length(a.velocity) == 0 || distance(a.position, other.position) >
                                           length(a.half_extents) + length(other.half_extents) +
                                               2 * s.prediction_steps * s.prediction_step * s.max_speed)
            continue;
        bool found = false;
        for (int i = 0; i <= s.prediction_steps && !found; ++i)
            for (int j = 0; j <= s.prediction_steps; ++j) {
                const Vec2 diff = (a.position + a.velocity * (i * s.prediction_step)) -
                                  (other.position + other.velocity * (j * s.prediction_step));
                const Vec2 extent = (a.half_extents + other.half_extents) * s.safety_multiplier;
                if (std::abs(diff.x) < extent.x && std::abs(diff.y) < extent.y) {
                    result.predictive += normalized(diff);
                    ++predicted;
                    found = true;
                    break;
                }
            }
    }
    if (predicted)
        result.predictive = result.predictive / float(predicted);
    const Vec2 offset = target - a.position;
    const float speed = s.max_speed * (arrival ? std::min(1.f, length(offset) / s.arrival_radius) : 1.f);
    result.seek = limited(normalized(offset) * speed - a.velocity, s.max_acceleration);
    Vec2 force = result.border + result.collision;
    if (length(result.border) == 0 && length(result.collision) == 0)
        force = length(result.predictive) > 0 ? result.predictive : result.seek;
    result.acceleration = limited(force, s.max_acceleration);
    return result;
}
} // namespace kinetra

namespace kinetra {
Vec2 tangent_steer(const Body &a, Vec2 target, std::span<const Body> neighbors,
                   float speed, float force, float horizon, bool arrival) {
    validate(a);
    if (!finite(target) || !std::isfinite(speed) || speed <= 0 ||
        !std::isfinite(force) || force <= 0 || !std::isfinite(horizon) || horizon <= 0 || horizon > 64)
        throw std::invalid_argument("Invalid tangent steering settings");
    const Vec2 desired = normalized(target - a.position);
    Vec2 repulsion{};
    const float radius = a.shape == Shape::circle ? a.radius : length(a.half_extents);
    for (const auto &other : neighbors) {
        validate(other);
        const float other_radius = other.shape == Shape::circle ? other.radius : length(other.half_extents);
        const float safety = radius + other_radius;
        const Vec2 offset = a.position - other.position;
        const float d = length(offset);
        if (d >= safety + speed * horizon || d < 0.00001f) continue;
        const float angle = std::atan2(offset.y, offset.x) + std::asin(std::clamp(safety / d, -1.f, 1.f));
        const Vec2 tangent = other.position + Vec2{std::cos(angle), std::sin(angle)} * safety;
        const Vec2 away = a.position - tangent;
        if (length(away) < 0.00001f) continue;
        const float alpha = std::atan2(-away.y, -away.x) - std::atan2(desired.y, desired.x);
        const float wrapped = std::remainder(alpha, 6.283185307f);
        repulsion += normalized(away) * (std::exp(.1f * wrapped) * .1f * (safety + radius - length(away)));
    }
    const float slow = std::max(.01f, radius * 2 * .45f);
    const float desired_speed = speed * (arrival ? std::min(1.f, distance(a.position,target)/slow) : 1.f);
    return limited(normalized(desired + repulsion) * desired_speed - a.velocity, force);
}
}
