#pragma once

#include "spatial_grid.h"
#include <optional>

namespace kinetra {

enum class Shape { circle, box };
enum class Motion { fixed, controlled, dynamic };

/// World units, seconds and radians; positive angles rotate counterclockwise.
/// Controlled bodies receive translation impulses/correction but no collision torque
/// or restitution. Dynamic bodies receive linear and angular impulses. Fixed bodies
/// never move. Half extents apply to boxes, radius to circles; all must be positive.
struct Body {
    Shape shape{Shape::circle};
    Motion motion{Motion::dynamic};
    Vec2 position{}, velocity{}, acceleration{};
    Vec2 half_extents{0.5f, 0.5f};
    float radius{0.5f}, angle{}, angular_velocity{};
    float mass{1}, restitution{0.4f}, friction{0.3f};
};

struct Contact {
    Vec2 normal; // From the first body toward the second; correction moves them apart.
    Vec2 point;
    float penetration;
};

Aabb bounds(const Body &body);
/// Detect circle/circle, circle/OBB or OBB/OBB contact, including touching shapes.
std::optional<Contact> collide(const Body &a, const Body &b);
/// Resolve an existing contact using the same impulses and correction as World::step.
void resolve_contact(Body &a, Body &b, const Contact &contact);
/// Apply a world-space impulse at a point, including angular momentum for dynamic bodies.
void apply_body_impulse(Body &body, Vec2 impulse, Vec2 point);
/// Semi-implicit integration; damping is a nonnegative linear drag rate per time unit.
void integrate(Body &body, float seconds, float damping = 0);
/// Earliest segment hit fraction, including a start inside the circle; nullopt on miss.
std::optional<float> segment_circle(Vec2 start, Vec2 end, Vec2 center, float radius);
/// Validate finite state, positive shape/mass and material ranges; throws invalid_argument.
void validate(const Body &body);

/// Discrete impulse simulation with a spatial broad phase. Body indices remain stable
/// until clear(); no removal API. Acceleration persists until changed by the caller.
/// step() accepts (0, 0.1] seconds; use a fixed timestep. No continuous collision detection.
class World {
  public:
    explicit World(float cell_size = 4) : grid_(cell_size) {}
    std::size_t add(Body body);
    void clear() {
        bodies_.clear();
        candidate_pairs_ = 0;
    }
    Body &body(std::size_t id) { return bodies_.at(id); }
    const Body &body(std::size_t id) const { return bodies_.at(id); }
    std::span<const Body> bodies() const { return bodies_; }
    void step(float seconds);
    Vec2 gravity{};
    std::size_t candidate_pairs() const { return candidate_pairs_; }

  private:
    std::vector<Body> bodies_;
    SpatialGrid grid_;
    std::size_t candidate_pairs_{};
};

} // namespace kinetra
