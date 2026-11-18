#include <kinetra/steering.h>
#include <stdexcept>

namespace kinetra {
Vec2 constrain_borders(const Body &a, Vec2 acceleration, std::span<const Segment> borders,
                       const PrioritySteering &s) {
    validate(a);
    if (a.shape != Shape::box || a.angle != 0 || !finite(acceleration))
        throw std::invalid_argument("Border steering requires an axis-aligned box and finite acceleration");
    for (float v :
         {s.max_speed, s.max_acceleration, s.border_distance, s.border_strength, s.border_lookahead})
        if (!std::isfinite(v) || v <= 0)
            throw std::invalid_argument("Border steering settings must be finite and positive");
    acceleration = limited(acceleration, s.max_acceleration);
    Vec2 lower{-s.max_acceleration, -s.max_acceleration};
    Vec2 upper{s.max_acceleration, s.max_acceleration};
    for (const auto &edge : borders) {
        if (!finite(edge.start) || !finite(edge.end) ||
            (edge.start.x != edge.end.x && edge.start.y != edge.end.y))
            throw std::invalid_argument("Border steering requires finite axis-aligned segments");
        // Minkowski expansion includes the rectangular footprint and finite end caps.
        const Vec2 center = (edge.start + edge.end) / 2;
        const Vec2 extent{std::abs(edge.end.x - edge.start.x) / 2 + a.half_extents.x,
                          std::abs(edge.end.y - edge.start.y) / 2 + a.half_extents.y};
        const Vec2 offset = a.position - center;
        const Vec2 gap{std::abs(offset.x) - extent.x, std::abs(offset.y) - extent.y};
        const bool x_axis = gap.x >= gap.y;
        const float d = x_axis ? gap.x : gap.y;
        const float sign = (x_axis ? offset.x : offset.y) >= 0 ? 1.f : -1.f;
        const float velocity = (x_axis ? a.velocity.x : a.velocity.y) * sign;
        // Brake before contact. At rest a touching parking goal stays valid; only
        // actual penetration requests an outward velocity, continuously at d=0.
        const float clearance = std::max(0.f, d);
        const float stopping_speed =
            std::sqrt(s.max_acceleration * s.max_acceleration + 2 * s.max_acceleration * clearance) -
            s.max_acceleration;
        const float inward = std::min(clearance / std::max(1.f, s.border_lookahead), stopping_speed);
        const float t = std::clamp(-d / s.border_distance, 0.f, 1.f);
        const float outward = std::min(s.max_speed, s.border_strength * t * t * (3 - 2 * t));
        const float correction =
            std::clamp(outward - inward - velocity, -s.max_acceleration, s.max_acceleration);
        float &lo = x_axis ? lower.x : lower.y;
        float &hi = x_axis ? upper.x : upper.y;
        if (sign > 0)
            lo = std::max(lo, correction);
        else
            hi = std::min(hi, -correction);
    }
    // Opposing constraints can be infeasible in an overlap; choose their midpoint
    // rather than depending on segment order or summing unbounded reactions.
    auto constrain = [](float value, float lo, float hi) {
        return lo <= hi ? std::clamp(value, lo, hi) : (lo + hi) / 2;
    };
    const Vec2 constrained{constrain(acceleration.x, lower.x, upper.x),
                           constrain(acceleration.y, lower.y, upper.y)};
    if (length(constrained) <= s.max_acceleration)
        return constrained;
    const Vec2 required{constrain(0, lower.x, upper.x), constrain(0, lower.y, upper.y)};
    if (length(required) >= s.max_acceleration)
        return limited(required, s.max_acceleration);
    // Spend the remaining acceleration budget on tangent/goal steering without
    // scaling away a feasible normal braking constraint.
    const Vec2 extra = constrained - required;
    const float b = dot(required, extra);
    const float c = dot(required, required) - s.max_acceleration * s.max_acceleration;
    const float t = (-b + std::sqrt(std::max(0.f, b * b - dot(extra, extra) * c))) / dot(extra, extra);
    return required + extra * std::clamp(t, 0.f, 1.f);
}

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
        if (length(a.velocity) == 0 ||
            distance(a.position, other.position) >
                length(a.half_extents) + length(other.half_extents) +
                    static_cast<float>(2 * s.prediction_steps) * s.prediction_step * s.max_speed)
            continue;
        bool found = false;
        for (int i = 0; i <= s.prediction_steps && !found; ++i)
            for (int j = 0; j <= s.prediction_steps; ++j) {
                const Vec2 diff =
                    (a.position + a.velocity * (static_cast<float>(i) * s.prediction_step)) -
                    (other.position + other.velocity * (static_cast<float>(j) * s.prediction_step));
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
    const Vec2 force = length(result.collision) > 0    ? result.collision
                       : length(result.predictive) > 0 ? result.predictive
                                                       : result.seek;
    const Vec2 acceleration = limited(force, s.max_acceleration);
    result.acceleration = constrain_borders(a, acceleration, borders, s);
    result.border = result.acceleration - acceleration;
    return result;
}
} // namespace kinetra

namespace kinetra {
Vec2 tangent_steer(const Body &a, Vec2 target, std::span<const Body> neighbors, float speed, float force,
                   float horizon, bool arrival) {
    validate(a);
    if (!finite(target) || !std::isfinite(speed) || speed <= 0 || !std::isfinite(force) || force <= 0 ||
        !std::isfinite(horizon) || horizon <= 0 || horizon > 64)
        throw std::invalid_argument("Invalid tangent steering settings");
    const Vec2 desired = normalized(target - a.position);
    Vec2 repulsion{};
    const float radius = bounding_radius(a);
    for (const auto &other : neighbors) {
        validate(other);
        const float other_radius = bounding_radius(other);
        const float safety = radius + other_radius;
        const Vec2 offset = a.position - other.position;
        const float d = length(offset);
        if (d >= safety + speed * horizon || d < 0.00001f)
            continue;
        const float angle = std::atan2(offset.y, offset.x) + std::asin(std::clamp(safety / d, -1.f, 1.f));
        const Vec2 tangent = other.position + Vec2{std::cos(angle), std::sin(angle)} * safety;
        const Vec2 away = a.position - tangent;
        if (length(away) < 0.00001f)
            continue;
        const float alpha = std::atan2(-away.y, -away.x) - std::atan2(desired.y, desired.x);
        const float wrapped = std::remainder(alpha, 6.283185307f);
        repulsion += normalized(away) * (std::exp(.1f * wrapped) * .1f * (safety + radius - length(away)));
    }
    const float slow = std::max(.01f, radius * 2 * .45f);
    const float desired_speed = speed * (arrival ? std::min(1.f, distance(a.position, target) / slow) : 1.f);
    return limited(normalized(desired + repulsion) * desired_speed - a.velocity, force);
}
} // namespace kinetra
