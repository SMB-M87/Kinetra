#include <array>
#include <kinetra/physics.h>
#include <limits>
#include <stdexcept>

namespace kinetra {
namespace {

float inverse_mass(const Body &b) {
    return b.motion == Motion::fixed ? 0 : 1 / b.mass;
}
float inverse_inertia(const Body &b) {
    if (b.motion != Motion::dynamic)
        return 0;
    const float inertia =
        b.shape == Shape::circle
            ? b.mass * b.radius * b.radius / 2
            : b.mass * (b.half_extents.x * b.half_extents.x + b.half_extents.y * b.half_extents.y) / 3;
    return 1 / inertia;
}

std::array<Vec2, 2> axes(const Body &b) {
    const Vec2 x = rotated({1, 0}, b.angle);
    return {x, perpendicular(x)};
}

float projected_radius(const Body &b, Vec2 axis) {
    const auto basis = axes(b);
    return std::abs(dot(axis, basis[0])) * b.half_extents.x +
           std::abs(dot(axis, basis[1])) * b.half_extents.y;
}

std::optional<Contact> circles(const Body &a, const Body &b) {
    const Vec2 delta = b.position - a.position;
    const float distance = length(delta), sum = a.radius + b.radius;
    if (distance > sum)
        return {};
    const Vec2 normal = distance > 0.000001f ? delta / distance : Vec2{1, 0};
    return Contact{normal, a.position + normal * (a.radius - (sum - distance) / 2), sum - distance};
}

std::optional<Contact> circle_box(const Body &circle, const Body &box) {
    const Vec2 local = rotated(circle.position - box.position, -box.angle);
    const Vec2 closest{std::clamp(local.x, -box.half_extents.x, box.half_extents.x),
                       std::clamp(local.y, -box.half_extents.y, box.half_extents.y)};
    const Vec2 delta = closest - local;
    const float distance = length(delta);
    if (distance > circle.radius)
        return {};
    Vec2 normal;
    Vec2 point = closest;
    float penetration;
    if (distance > 0.000001f) {
        normal = delta / distance;
        penetration = circle.radius - distance;
    } else {
        // A contained circle must exit via the nearest face. The contact normal is
        // opposite that exit because correction moves the first body against it.
        const float dx = box.half_extents.x - std::abs(local.x);
        const float dy = box.half_extents.y - std::abs(local.y);
        const Vec2 exit = dx < dy ? Vec2{local.x >= 0 ? 1.f : -1.f, 0} : Vec2{0, local.y >= 0 ? 1.f : -1.f};
        normal = -exit;
        penetration = circle.radius + std::min(dx, dy);
        point = local + exit * std::min(dx, dy);
    }
    return Contact{rotated(normal, box.angle), box.position + rotated(point, box.angle), penetration};
}

/// SAT gives the minimum translation axis. Clipping the incident face against the
/// reference face yields a representative contact without arbitrary corner torque.
std::optional<Contact> boxes(const Body &a, const Body &b) {
    const auto aa = axes(a), ba = axes(b);
    const std::array<Vec2, 4> candidates{aa[0], aa[1], ba[0], ba[1]};
    float penetration = std::numeric_limits<float>::max();
    int reference_axis = 0;
    Vec2 normal;
    for (int i = 0; i < 4; ++i) {
        const Vec2 axis = candidates[i];
        const float separation = dot(b.position - a.position, axis);
        const float ar = projected_radius(a, axis), br = projected_radius(b, axis);
        const float overlap = ar + br - std::abs(separation);
        if (overlap < 0)
            return {};
        if (overlap < penetration) {
            penetration = overlap;
            normal = separation >= 0 ? axis : -axis;
            reference_axis = i;
        }
    }
    const Body &ref = reference_axis < 2 ? a : b;
    const Body &inc = reference_axis < 2 ? b : a;
    const Vec2 ref_normal = reference_axis < 2 ? normal : -normal;
    const auto ref_axes = axes(ref), inc_axes = axes(inc);
    const int face_axis = reference_axis % 2;
    const Vec2 tangent = ref_axes[1 - face_axis];
    const float face_extent = face_axis == 0 ? ref.half_extents.x : ref.half_extents.y;
    const float side_extent = face_axis == 0 ? ref.half_extents.y : ref.half_extents.x;
    const Vec2 face_center = ref.position + ref_normal * face_extent;
    const int incident_axis =
        std::abs(dot(inc_axes[0], ref_normal)) > std::abs(dot(inc_axes[1], ref_normal)) ? 0 : 1;
    const Vec2 inc_normal =
        inc_axes[incident_axis] * (dot(inc_axes[incident_axis], ref_normal) > 0 ? -1.f : 1.f);
    const float inc_extent = incident_axis == 0 ? inc.half_extents.x : inc.half_extents.y;
    const float inc_side = incident_axis == 0 ? inc.half_extents.y : inc.half_extents.x;
    const Vec2 center = inc.position + inc_normal * inc_extent;
    Vec2 p = center - inc_axes[1 - incident_axis] * inc_side;
    Vec2 q = center + inc_axes[1 - incident_axis] * inc_side;
    // Clip the incident edge to the reference face's side planes.
    for (float sign : {-1.f, 1.f}) {
        const Vec2 side = tangent * sign;
        const float pd = dot(p - ref.position, side) - side_extent;
        const float qd = dot(q - ref.position, side) - side_extent;
        if (pd > 0 && qd > 0)
            return Contact{normal, (a.position + b.position) / 2, penetration};
        if ((pd > 0) != (qd > 0)) {
            const Vec2 intersection = p + (q - p) * (pd / (pd - qd));
            if (pd > 0)
                p = intersection;
            else
                q = intersection;
        }
    }
    Vec2 sum{};
    int count = 0;
    for (Vec2 point : {p, q}) {
        const float depth = dot(point - face_center, ref_normal);
        if (depth <= 0.00001f) {
            sum += point - ref_normal * (depth / 2);
            ++count;
        }
    }
    return Contact{normal, count ? sum / float(count) : (p + q) / 2, penetration};
}

void apply_impulse(Body &a, Body &b, Vec2 impulse, Vec2 ra, Vec2 rb) {
    a.velocity -= impulse * inverse_mass(a);
    b.velocity += impulse * inverse_mass(b);
    a.angular_velocity -= cross(ra, impulse) * inverse_inertia(a);
    b.angular_velocity += cross(rb, impulse) * inverse_inertia(b);
}

Vec2 point_velocity(const Body &b, Vec2 arm) {
    return b.motion == Motion::fixed ? Vec2{} : b.velocity + perpendicular(arm) * b.angular_velocity;
}

void resolve(Body &a, Body &b, const Contact &c) {
    const float ma = inverse_mass(a), mb = inverse_mass(b), total = ma + mb;
    if (total == 0)
        return;
    const Vec2 ra = c.point - a.position, rb = c.point - b.position;
    const Vec2 relative = point_velocity(b, rb) - point_velocity(a, ra);
    const float speed = dot(relative, c.normal);
    if (speed < 0) {
        const float ca = cross(ra, c.normal), cb = cross(rb, c.normal);
        const float denominator = total + ca * ca * inverse_inertia(a) + cb * cb * inverse_inertia(b);
        // Suppress low-speed bounce to reduce jitter at resting contacts.
        const float bounce =
            (a.motion == Motion::controlled || b.motion == Motion::controlled || speed > -0.5f)
                ? 0
                : std::min(a.restitution, b.restitution);
        const float impulse = -(1 + bounce) * speed / denominator;
        apply_impulse(a, b, c.normal * impulse, ra, rb);
        const Vec2 tangent = perpendicular(c.normal);
        const Vec2 after = point_velocity(b, rb) - point_velocity(a, ra);
        const float ta = cross(ra, tangent), tb = cross(rb, tangent);
        const float friction = std::sqrt(a.friction * b.friction) * impulse;
        const float jt = std::clamp(-dot(after, tangent) /
                                        (total + ta * ta * inverse_inertia(a) + tb * tb * inverse_inertia(b)),
                                    -friction, friction);
        apply_impulse(a, b, tangent * jt, ra, rb);
    }
    const Vec2 correction = c.normal * (std::max(c.penetration - 0.001f, 0.f) * 0.6f / total);
    a.position -= correction * ma;
    b.position += correction * mb;
}

} // namespace

void validate(const Body &b) {
    if ((b.shape != Shape::circle && b.shape != Shape::box) ||
        (b.motion != Motion::fixed && b.motion != Motion::controlled && b.motion != Motion::dynamic) ||
        !finite(b.position) || !finite(b.velocity) || !finite(b.acceleration) || !finite(b.half_extents) ||
        !std::isfinite(b.angle) || !std::isfinite(b.angular_velocity) || !std::isfinite(b.mass) ||
        !std::isfinite(b.radius) || !std::isfinite(b.restitution) || !std::isfinite(b.friction) ||
        b.mass < 0.0001f || b.radius < 0.0001f || b.half_extents.x < 0.0001f || b.half_extents.y < 0.0001f ||
        b.restitution < 0 || b.restitution > 1 || b.friction < 0 || b.friction > 1)
        throw std::invalid_argument(
            "Invalid body: finite state, positive mass/dimensions and material values in [0,1] required");
}

Aabb bounds(const Body &b) {
    const Vec2 extent = b.shape == Shape::circle
                            ? Vec2{b.radius, b.radius}
                            : Vec2{projected_radius(b, {1, 0}), projected_radius(b, {0, 1})};
    return {b.position - extent, b.position + extent};
}

std::optional<Contact> collide(const Body &a, const Body &b) {
    validate(a);
    validate(b);
    if (a.shape == Shape::circle && b.shape == Shape::circle)
        return circles(a, b);
    if (a.shape == Shape::box && b.shape == Shape::box)
        return boxes(a, b);
    if (a.shape == Shape::circle)
        return circle_box(a, b);
    auto c = circle_box(b, a);
    if (c)
        c->normal = -c->normal;
    return c;
}

void resolve_contact(Body &a, Body &b, const Contact &contact) {
    validate(a);
    validate(b);
    if (!finite(contact.normal) || !finite(contact.point) || !std::isfinite(contact.penetration) ||
        contact.penetration < 0 || std::abs(length(contact.normal) - 1) > 0.001f)
        throw std::invalid_argument("Contact requires a unit normal and nonnegative finite penetration");
    resolve(a, b, contact);
}

void apply_body_impulse(Body &body, Vec2 impulse, Vec2 point) {
    validate(body);
    if (!finite(impulse) || !finite(point))
        throw std::invalid_argument("Impulse and application point must be finite");
    body.velocity += impulse * inverse_mass(body);
    body.angular_velocity += cross(point - body.position, impulse) * inverse_inertia(body);
}

void integrate(Body &body, float seconds, float damping) {
    validate(body);
    if (!std::isfinite(seconds) || seconds < 0 || !std::isfinite(damping) || damping < 0)
        throw std::invalid_argument("Integration requires finite nonnegative time and damping");
    if (body.motion == Motion::fixed)
        return;
    body.velocity = (body.velocity + body.acceleration * seconds) * std::max(0.f, 1 - damping * seconds);
    body.position += body.velocity * seconds;
    body.angle = std::remainder(body.angle + body.angular_velocity * seconds, 6.28318530718f);
}

std::optional<float> segment_circle(Vec2 start, Vec2 end, Vec2 center, float radius) {
    if (!finite(start) || !finite(end) || !finite(center) || !std::isfinite(radius) || radius < 0)
        throw std::invalid_argument("Segment and circle must be finite with a nonnegative radius");
    // Double intermediates preserve grazing hits and tiny relative movements.
    // A speed threshold can discard a real hit when target and projectile co-move.
    const double dx = double(end.x) - start.x, dy = double(end.y) - start.y;
    const double ox = double(start.x) - center.x, oy = double(start.y) - center.y;
    const double c = ox * ox + oy * oy - double(radius) * radius;
    if (c <= 0)
        return 0;
    const double a = dx * dx + dy * dy, b = ox * dx + oy * dy;
    const double discriminant = b * b - a * c;
    if (a == 0 || b >= 0 || discriminant < 0)
        return std::nullopt;
    const double fraction = c / (-b + std::sqrt(discriminant));
    return fraction >= 0 && fraction <= 1 ? std::optional<float>{float(fraction)} : std::nullopt;
}

std::size_t World::add(Body b) {
    validate(b);
    bodies_.push_back(b);
    return bodies_.size() - 1;
}

void World::step(float dt) {
    if (!std::isfinite(dt) || dt <= 0 || dt > 0.1f || !finite(gravity))
        throw std::invalid_argument("World timestep must be in (0, 0.1] and gravity finite");
    for (const auto &b : bodies_)
        validate(b);
    candidate_pairs_ = 0;
    // Two integration substeps reduce penetration; the public timestep remains fixed.
    for (int substep = 0; substep < 2; ++substep) {
        for (auto &b : bodies_) {
            if (b.motion == Motion::fixed)
                continue;
            b.velocity += gravity * (dt / 2);
            integrate(b, dt / 2);
        }
        // Rebuild after correction passes as correction can create new contacts.
        for (int pass = 0; pass < 6; ++pass) {
            std::vector<Aabb> boxes;
            boxes.reserve(bodies_.size());
            for (const auto &b : bodies_)
                boxes.push_back(bounds(b));
            grid_.rebuild(boxes);
            for (std::size_t i = 0; i < bodies_.size(); ++i) {
                // Only movers initiate queries. Fixed bodies remain indexed, including
                // those inserted before a mover; moving pairs are still visited once.
                if (bodies_[i].motion == Motion::fixed)
                    continue;
                for (auto j : grid_.query(boxes[i])) {
                    if (j == i || (j < i && bodies_[j].motion != Motion::fixed))
                        continue;
                    ++candidate_pairs_;
                    const auto contact = collide(bodies_[i], bodies_[j]);
                    if (contact)
                        resolve(bodies_[i], bodies_[j], *contact);
                }
            }
        }
    }
}

} // namespace kinetra
