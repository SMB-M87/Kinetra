#include <algorithm>
#include <array>
#include <kinetra/simulation/physics.h>
#include <limits>
#include <stdexcept>

namespace kinetra {
namespace {

constexpr float contact_slop = 0.001f;

float inner_radius(const Body &body) {
    if (body.shape == Shape::circle)
        return body.radius;
    if (body.shape == Shape::box)
        return std::min(body.half_extents.x, body.half_extents.y);
    float radius = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < body.vertices.size(); ++i) {
        const auto a = body.vertices[i], b = body.vertices[(i + 1) % body.vertices.size()];
        radius = std::min(radius, cross(a, b) / length(b - a));
    }
    return radius;
}

float inverse_mass(const Body &b) {
    return b.motion == Motion::fixed ? 0 : 1 / b.mass;
}
float inverse_inertia(const Body &b) {
    if (b.motion != Motion::dynamic)
        return 0;
    if (b.shape == Shape::polygon) {
        double area = 0, moment = 0;
        for (std::size_t i = 0; i < b.vertices.size(); ++i) {
            const auto a = b.vertices[i], c = b.vertices[(i + 1) % b.vertices.size()];
            const double weight = cross(a, c);
            area += weight;
            moment += weight * (dot(a, a) + dot(a, c) + dot(c, c));
        }
        return float(6 * area / (b.mass * moment));
    }
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

std::vector<Vec2> hull(const Body &body) {
    std::vector<Vec2> points = body.vertices;
    if (body.shape == Shape::box) {
        const auto e = body.half_extents;
        points = {{-e.x, -e.y}, {e.x, -e.y}, {e.x, e.y}, {-e.x, e.y}};
    }
    for (auto &p : points)
        p = body.position + rotated(p, body.angle);
    return points;
}

std::optional<Contact> circle_polygon(const Body &circle, const Body &polygon) {
    const auto points = hull(polygon);
    float largest_gap = -std::numeric_limits<float>::max();
    float nearest = std::numeric_limits<float>::max();
    Vec2 face_normal{}, closest{};
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto a = points[i], b = points[(i + 1) % points.size()];
        const auto edge = b - a;
        const auto outward = -normalized(perpendicular(edge));
        const float gap = dot(circle.position - a, outward);
        if (gap > largest_gap) {
            largest_gap = gap;
            face_normal = outward;
        }
        const auto point = a + edge * std::clamp(dot(circle.position - a, edge) / dot(edge, edge), 0.f, 1.f);
        const float distance = length(point - circle.position);
        if (distance < nearest) {
            nearest = distance;
            closest = point;
        }
    }
    if (largest_gap <= 0)
        return Contact{-face_normal, circle.position - face_normal * largest_gap,
                       circle.radius - largest_gap};
    if (nearest > circle.radius)
        return {};
    return Contact{normalized(closest - circle.position), closest, circle.radius - nearest};
}

// SAT selects a reference edge; clip the incident edge to its side planes.
// Keep the existing representative-point contact contract (no stacking manifold).
std::optional<Contact> polygon_contact(const Body &a, const Body &b, float tolerance = 0) {
    const auto ap = hull(a), bp = hull(b);
    float gap = -std::numeric_limits<float>::max();
    std::size_t reference = 0;
    bool from_a = true;
    Vec2 normal{};
    for (bool first : {true, false}) {
        const auto &points = first ? ap : bp;
        const auto &other = first ? bp : ap;
        for (std::size_t i = 0; i < points.size(); ++i) {
            const auto outward = -normalized(perpendicular(points[(i + 1) % points.size()] - points[i]));
            float separation = std::numeric_limits<float>::max();
            for (auto point : other)
                separation = std::min(separation, dot(point - points[i], outward));
            if (separation > tolerance)
                return {};
            if (separation > gap) {
                gap = separation;
                normal = outward;
                reference = i;
                from_a = first;
            }
        }
    }
    const auto &ref = from_a ? ap : bp;
    const auto &inc = from_a ? bp : ap;
    std::size_t incident = 0;
    float alignment = std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < inc.size(); ++i) {
        const auto outward = -normalized(perpendicular(inc[(i + 1) % inc.size()] - inc[i]));
        if (dot(outward, normal) < alignment) {
            alignment = dot(outward, normal);
            incident = i;
        }
    }
    const auto start = ref[reference], end = ref[(reference + 1) % ref.size()];
    const auto tangent = normalized(end - start);
    Vec2 p = inc[incident], q = inc[(incident + 1) % inc.size()];
    for (bool upper : {false, true}) {
        const Vec2 axis = upper ? tangent : -tangent;
        const Vec2 origin = upper ? end : start;
        const auto side_distance = [&](Vec2 point) {
            return (double(point.x) - origin.x) * axis.x + (double(point.y) - origin.y) * axis.y;
        };
        const double pd = side_distance(p), qd = side_distance(q);
        if (pd > 0 && qd > 0)
            return Contact{from_a ? normal : -normal, (p + q) / 2, std::max(0.f, -gap)};
        if ((pd > 0) != (qd > 0)) {
            // Double interpolation avoids spurious torque on centered face impacts.
            const double fraction = pd / (pd - qd);
            const Vec2 intersection{float(double(p.x) + (double(q.x) - p.x) * fraction),
                                    float(double(p.y) + (double(q.y) - p.y) * fraction)};
            if (pd > 0)
                p = intersection;
            else
                q = intersection;
        }
    }
    Vec2 sum{};
    int count = 0;
    for (auto point : {p, q}) {
        const float depth = dot(point - start, normal);
        if (depth <= std::max(tolerance, 0.00001f)) {
            sum += point - normal * (depth / 2);
            ++count;
        }
    }
    return Contact{from_a ? normal : -normal, count ? sum / float(count) : (p + q) / 2, std::max(0.f, -gap)};
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
    const Vec2 correction = c.normal * (std::max(c.penetration - contact_slop, 0.f) * 0.6f / total);
    a.position -= correction * ma;
    b.position += correction * mb;
}

} // namespace

void validate(const Body &b) {
    if ((b.shape != Shape::circle && b.shape != Shape::box && b.shape != Shape::polygon) ||
        (b.motion != Motion::fixed && b.motion != Motion::controlled && b.motion != Motion::dynamic) ||
        !finite(b.position) || !finite(b.velocity) || !finite(b.acceleration) || !finite(b.half_extents) ||
        !std::isfinite(b.angle) || !std::isfinite(b.angular_velocity) || !std::isfinite(b.mass) ||
        !std::isfinite(b.radius) || !std::isfinite(b.restitution) || !std::isfinite(b.friction) ||
        b.mass < 0.0001f || b.radius < 0.0001f || b.half_extents.x < 0.0001f || b.half_extents.y < 0.0001f ||
        b.restitution < 0 || b.restitution > 1 || b.friction < 0 || b.friction > 1)
        throw std::invalid_argument(
            "Invalid body: finite state, positive mass/dimensions and material values in [0,1] required");
    if (b.shape == Shape::polygon) {
        if (b.vertices.size() < 3 || b.vertices.size() > 32)
            throw std::invalid_argument("Polygon requires 3..32 convex CCW vertices");
        double area = 0;
        Vec2 centroid{};
        for (std::size_t i = 0; i < b.vertices.size(); ++i) {
            const auto a = b.vertices[i], c = b.vertices[(i + 1) % b.vertices.size()];
            if (!finite(a) || length(c - a) < 0.0001f)
                throw std::invalid_argument("Polygon vertices must be finite and edges nondegenerate");
            for (std::size_t j = 0; j < b.vertices.size(); ++j)
                if (j != i && j != (i + 1) % b.vertices.size() && cross(c - a, b.vertices[j] - a) <= 0)
                    throw std::invalid_argument("Polygon must be strictly convex and counterclockwise");
            const float weight = cross(a, c);
            area += weight;
            centroid += (a + c) * weight;
        }
        if (!std::isfinite(area) || area < 0.00000001 || !finite(centroid) ||
            length(centroid / float(3 * area)) > 0.0001f * std::max(1.f, bounding_radius(b)))
            throw std::invalid_argument("Polygon must have positive area and be centered at its centroid");
    }
}

Vec2 local_extents(const Body &b) {
    if (b.shape == Shape::circle)
        return {b.radius, b.radius};
    if (b.shape == Shape::box)
        return b.half_extents;
    Vec2 extent{};
    for (auto p : b.vertices) {
        extent.x = std::max(extent.x, std::abs(p.x));
        extent.y = std::max(extent.y, std::abs(p.y));
    }
    return extent;
}

float bounding_radius(const Body &b) {
    if (b.shape == Shape::circle)
        return b.radius;
    if (b.shape == Shape::box)
        return length(b.half_extents);
    float radius = 0;
    for (auto p : b.vertices)
        radius = std::max(radius, length(p));
    return radius;
}

Body regular_polygon(int sides, float radius) {
    if (sides < 3 || sides > 32 || !std::isfinite(radius) || radius < 0.0001f)
        throw std::invalid_argument("Regular polygon requires 3..32 sides and positive finite radius");
    Body body;
    body.shape = Shape::polygon;
    for (int i = 0; i < sides; ++i)
        body.vertices.push_back(rotated({radius, 0}, 6.28318530718f * float(i) / float(sides)));
    validate(body);
    return body;
}

Aabb bounds(const Body &b) {
    if (b.shape == Shape::polygon) {
        const auto points = hull(b);
        Aabb result{points.front(), points.front()};
        for (auto p : points) {
            result.min.x = std::min(result.min.x, p.x);
            result.min.y = std::min(result.min.y, p.y);
            result.max.x = std::max(result.max.x, p.x);
            result.max.y = std::max(result.max.y, p.y);
        }
        return result;
    }
    const Vec2 extent = b.shape == Shape::circle
                            ? Vec2{b.radius, b.radius}
                            : Vec2{projected_radius(b, {1, 0}), projected_radius(b, {0, 1})};
    return {b.position - extent, b.position + extent};
}

std::optional<Contact> collide(const Body &a, const Body &b) {
    validate(a);
    validate(b);
    if (a.shape == Shape::polygon || b.shape == Shape::polygon) {
        if (a.shape == Shape::circle)
            return circle_polygon(a, b);
        if (b.shape == Shape::circle) {
            auto contact = circle_polygon(b, a);
            if (contact)
                contact->normal = -contact->normal;
            return contact;
        }
        return polygon_contact(a, b);
    }
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

std::optional<SweepContact> sweep_circle(const Body &circle, Vec2 end, const Body &obstacle) {
    validate(circle);
    validate(obstacle);
    if (circle.shape != Shape::circle || !finite(end))
        throw std::invalid_argument("Circle sweep requires a circle and a finite endpoint");
    if (auto contact = collide(circle, obstacle))
        return SweepContact{0, *contact};
    if (obstacle.shape == Shape::circle) {
        const auto fraction =
            segment_circle(circle.position, end, obstacle.position, circle.radius + obstacle.radius);
        if (!fraction)
            return {};
        const Vec2 center = circle.position + (end - circle.position) * *fraction;
        const Vec2 normal = normalized(obstacle.position - center);
        return SweepContact{*fraction, {normal, obstacle.position - normal * obstacle.radius, 0}};
    }
    // The circle-expanded convex hull consists of offset faces and round vertex arcs.
    // Face hits must project onto the original edge; vertex circles handle corners.
    const auto points = hull(obstacle);
    const Vec2 travel = end - circle.position;
    std::optional<SweepContact> earliest;
    auto consider = [&](float fraction, Vec2 normal, Vec2 point) {
        if (fraction >= 0 && fraction <= 1 && (!earliest || fraction < earliest->fraction))
            earliest = SweepContact{fraction, {normal, point, 0}};
    };
    for (std::size_t i = 0; i < points.size(); ++i) {
        const Vec2 start = points[i], finish = points[(i + 1) % points.size()];
        const Vec2 edge = finish - start;
        const float edge_length = length(edge);
        const Vec2 tangent = edge / edge_length, outward = -perpendicular(tangent);
        const double gap = double(dot(circle.position - start, outward)) - circle.radius;
        const double approach = dot(travel, outward);
        if (gap >= 0 && approach < 0) {
            const double fraction = -gap / approach;
            if (fraction >= 0 && fraction <= 1) {
                const Vec2 center = circle.position + travel * float(fraction);
                const float projection = dot(center - start, tangent);
                if (projection >= 0 && projection <= edge_length)
                    consider(float(fraction), -outward, start + tangent * projection);
            }
        }
        if (auto fraction = segment_circle(circle.position, end, start, circle.radius)) {
            const Vec2 center = circle.position + travel * *fraction;
            consider(*fraction, normalized(start - center), start);
        }
    }
    return earliest;
}

std::optional<SweepContact> sweep_translation(const Body &body, Vec2 end, const Body &obstacle) {
    validate(body);
    validate(obstacle);
    if (!finite(end))
        throw std::invalid_argument("Translation sweep requires a finite endpoint");
    if (body.shape == Shape::circle)
        return sweep_circle(body, end, obstacle);
    if (auto contact = collide(body, obstacle))
        return SweepContact{0, *contact};
    const Vec2 travel = end - body.position;
    if (obstacle.shape == Shape::circle) {
        // Hold the convex mover still and sweep the circle in relative coordinates.
        auto hit = sweep_circle(obstacle, obstacle.position - travel, body);
        if (hit) {
            hit->contact.normal = -hit->contact.normal;
            hit->contact.point += travel * hit->fraction;
        }
        return hit;
    }
    const auto moving = hull(body), fixed = hull(obstacle);
    double entry = 0, exit = 1;
    for (const auto *points : {&moving, &fixed}) {
        for (std::size_t i = 0; i < points->size(); ++i) {
            const auto axis = normalized(perpendicular((*points)[(i + 1) % points->size()] - (*points)[i]));
            auto project = [&](const std::vector<Vec2> &vertices) {
                double minimum = std::numeric_limits<double>::max(), maximum = -minimum;
                for (auto point : vertices) {
                    const double value = dot(point - body.position, axis);
                    minimum = std::min(minimum, value);
                    maximum = std::max(maximum, value);
                }
                return std::pair{minimum, maximum};
            };
            const auto [amin, amax] = project(moving);
            const auto [bmin, bmax] = project(fixed);
            const double speed = dot(travel, axis);
            if (speed == 0) {
                if (amax < bmin || bmax < amin)
                    return {};
                continue;
            }
            double begin = (bmin - amax) / speed, finish = (bmax - amin) / speed;
            if (begin > finish)
                std::swap(begin, finish);
            entry = std::max(entry, begin);
            exit = std::min(exit, finish);
            if (entry > exit)
                return {};
        }
    }
    Body at_impact = body;
    const float fraction = float(entry);
    at_impact.position += travel * fraction;
    const float scale = std::max({1.f, bounding_radius(body), bounding_radius(obstacle),
                                  std::abs(at_impact.position.x), std::abs(at_impact.position.y),
                                  std::abs(obstacle.position.x), std::abs(obstacle.position.y)});
    const auto contact =
        polygon_contact(at_impact, obstacle, 32 * std::numeric_limits<float>::epsilon() * scale);
    if (!contact)
        throw std::runtime_error(
            "Unable to reconstruct translation sweep contact at floating-point precision");
    auto impact_contact = *contact;
    impact_contact.penetration = 0;
    return SweepContact{fraction, impact_contact};
}

std::optional<SweepContact> sweep_translation(const Body &a, Vec2 end_a, const Body &b, Vec2 end_b) {
    if (!finite(end_a) || !finite(end_b))
        throw std::invalid_argument("Pair sweep requires finite endpoints");
    const Vec2 travel_b = end_b - b.position;
    auto hit = sweep_translation(a, end_a - travel_b, b);
    if (hit)
        hit->contact.point += travel_b * hit->fraction;
    return hit;
}

namespace {

bool same(Vec2 a, Vec2 b) {
    return a.x == b.x && a.y == b.y;
}

// Whether bounds() gives both bodies the same box, by the fields it reads for the shape, so
// that turning a fixed circle keeps the index. Comparing with == treats -0 and +0 as equal,
// which changes no overlap test.
bool same_geometry(const Body &a, const Body &b) {
    if (a.shape != b.shape || !same(a.position, b.position))
        return false;
    switch (a.shape) {
    case Shape::circle:
        return a.radius == b.radius;
    case Shape::box:
        return same(a.half_extents, b.half_extents) && a.angle == b.angle;
    case Shape::polygon:
        return a.angle == b.angle &&
               std::equal(a.vertices.begin(), a.vertices.end(), b.vertices.begin(), b.vertices.end(), same);
    }
    return false;
}

} // namespace

bool World::fixed_index_current() const {
    if (!fixed_index_valid_)
        return false;
    std::size_t k = 0;
    for (std::size_t i = 0; i < bodies_.size(); ++i) {
        if (bodies_[i].motion != Motion::fixed)
            continue;
        if (k == fixed_ids_.size() || fixed_ids_[k] != i || !same_geometry(bodies_[i], fixed_bodies_[k]))
            return false;
        ++k;
    }
    return k == fixed_ids_.size();
}

void World::index_fixed_bodies() {
    std::vector<std::size_t> ids;
    std::vector<Aabb> boxes;
    for (std::size_t i = 0; i < bodies_.size(); ++i)
        if (bodies_[i].motion == Motion::fixed) {
            ids.push_back(i);
            boxes.push_back(bounds(bodies_[i]));
        }
    fixed_grid_.rebuild(boxes); // Throws for a bad box and leaves the index invalid.
    fixed_bodies_.clear();
    for (std::size_t k = 0; k < ids.size(); ++k) {
        fixed_bodies_.push_back(bodies_[ids[k]]);
        boxes_[ids[k]] = boxes[k];
    }
    fixed_ids_ = std::move(ids);
    fixed_index_valid_ = true;
}

void World::candidates(Aabb area, bool with_fixed, std::vector<std::size_t> &out) const {
    out.clear();
    const auto moving = grid_.query(area);
    if (!with_fixed || fixed_ids_.empty()) {
        for (auto k : moving)
            out.push_back(moving_ids_[k]);
        return;
    }
    // Both lists ascend in body index, so merging them gives the sorted, unique list that
    // one grid over all bodies would return.
    const auto fixed = fixed_grid_.query(area);
    std::size_t f = 0, m = 0;
    while (f < fixed.size() || m < moving.size()) {
        if (m == moving.size() || (f < fixed.size() && fixed_ids_[fixed[f]] < moving_ids_[moving[m]]))
            out.push_back(fixed_ids_[fixed[f++]]);
        else
            out.push_back(moving_ids_[moving[m++]]);
    }
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
    sweep_budget_exhausted_ = false;
    const auto moving_count = std::count_if(bodies_.begin(), bodies_.end(),
                                            [](const Body &b) { return b.motion != Motion::fixed; });
    const std::size_t event_budget = 8 * std::max<std::size_t>(1, moving_count);
    // The caller may have changed a fixed body since the last step. The index itself is
    // rebuilt at the first pass, where a bad box throws as it would from one grid.
    // While the index is valid, boxes_ keeps the fixed boxes it was built from: a pass
    // writes only the boxes of moving bodies.
    boxes_.resize(bodies_.size());
    if (!fixed_index_current())
        fixed_index_valid_ = false;
    moving_ids_.clear();
    for (std::size_t i = 0; i < bodies_.size(); ++i)
        if (bodies_[i].motion != Motion::fixed)
            moving_ids_.push_back(i);
    // Fixed boxes are the same in every pass; only the moving bodies are indexed again.
    auto index_pass = [&](auto box_of) {
        if (!fixed_index_valid_)
            index_fixed_bodies();
        moving_boxes_.clear();
        for (auto i : moving_ids_) {
            boxes_[i] = box_of(bodies_[i]);
            moving_boxes_.push_back(boxes_[i]);
        }
        grid_.rebuild(moving_boxes_);
    };
    std::vector<std::size_t> found;
    auto velocity = [](const Body &b) { return b.motion == Motion::fixed ? Vec2{} : b.velocity; };
    auto sweep_supported = [](const Body &b) {
        return b.motion == Motion::fixed || b.shape == Shape::circle || b.angular_velocity == 0;
    };
    auto advance = [&](float seconds) {
        for (auto &b : bodies_) {
            if (b.motion == Motion::fixed)
                continue;
            b.position += b.velocity * seconds;
            b.angle = std::remainder(b.angle + b.angular_velocity * seconds, 6.28318530718f);
        }
    };
    // Integrate forces once per substep, then advance all bodies on one event timeline.
    for (int substep = 0; substep < 2; ++substep) {
        for (auto &b : bodies_) {
            if (b.motion == Motion::fixed)
                continue;
            b.velocity += gravity * (dt / 2);
            b.velocity += b.acceleration * (dt / 2);
        }
        float remaining = dt / 2;
        for (std::size_t event = 0; event < event_budget && remaining > 0; ++event) {
            // A fixed body's swept box is its plain box: it has no velocity.
            index_pass([&](const Body &b) {
                auto box = bounds(b);
                const Vec2 travel = b.velocity * remaining;
                box.min += Vec2{std::min(0.f, travel.x), std::min(0.f, travel.y)};
                box.max += Vec2{std::max(0.f, travel.x), std::max(0.f, travel.y)};
                return box;
            });
            std::optional<SweepContact> earliest;
            std::size_t first = 0, second = 0;
            for (std::size_t i = 0; i < bodies_.size(); ++i) {
                const auto &a = bodies_[i];
                if (!sweep_supported(a))
                    continue;
                // A fixed body skips the fixed candidates below, so it needs none.
                candidates(boxes_[i], a.motion != Motion::fixed, found);
                for (auto j : found) {
                    const auto &b = bodies_[j];
                    if (j <= i || !sweep_supported(b) ||
                        (a.motion == Motion::fixed && b.motion == Motion::fixed))
                        continue;
                    ++candidate_pairs_;
                    const auto hit = sweep_translation(a, a.position + velocity(a) * remaining, b,
                                                       b.position + velocity(b) * remaining);
                    if (!hit)
                        continue;
                    // Match resolve(): a spinning circle can have a tiny apparent
                    // linear approach while the actual contact point is separating.
                    // Testing a different velocity here can repeat a zero-time event
                    // that resolve() correctly leaves unchanged, starving the frame.
                    const float elapsed = remaining * hit->fraction;
                    const Vec2 position_a = a.position + velocity(a) * elapsed;
                    const Vec2 position_b = b.position + velocity(b) * elapsed;
                    const Vec2 va = point_velocity(a, hit->contact.point - position_a);
                    const Vec2 vb = point_velocity(b, hit->contact.point - position_b);
                    const float tolerance =
                        8 * std::numeric_limits<float>::epsilon() * std::max({1.f, length(va), length(vb)});
                    const float closing = dot(va - vb, hit->contact.normal);
                    // Persistent corner contacts can alternately reintroduce tiny
                    // approach velocities. Let the bounded discrete passes handle
                    // motion within positional slop instead of chasing convergence
                    // through thousands of zero-time CCD events. Scale down for tiny
                    // shapes so the tolerance cannot swallow their full thickness.
                    const float resting_slop =
                        std::min(contact_slop, 0.01f * std::min(inner_radius(a), inner_radius(b)));
                    if (hit->fraction == 0 && closing * remaining <= resting_slop)
                        continue;
                    // Stable pair order breaks ties.
                    if (closing > tolerance && (!earliest || hit->fraction < earliest->fraction)) {
                        earliest = hit;
                        first = i;
                        second = j;
                    }
                }
            }
            if (!earliest) {
                advance(remaining);
                remaining = 0;
                break;
            }
            const float elapsed = remaining * earliest->fraction;
            advance(elapsed);
            remaining -= elapsed;
            resolve(bodies_[first], bodies_[second], earliest->contact);
        }
        if (remaining > 0)
            sweep_budget_exhausted_ = true;
        // Rebuild after correction passes as correction can create new contacts.
        for (int pass = 0; pass < 6; ++pass) {
            index_pass([](const Body &b) { return bounds(b); });
            for (std::size_t i = 0; i < bodies_.size(); ++i) {
                // Only movers initiate queries. Fixed bodies remain indexed, including
                // those inserted before a mover; moving pairs are still visited once.
                // Boxes stay those of the pass's start, as one rebuild per pass gave.
                if (bodies_[i].motion == Motion::fixed)
                    continue;
                candidates(boxes_[i], true, found);
                for (auto j : found) {
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
