#pragma once

#include "spatial_grid.h"
#include <optional>

namespace kinetra {

enum class Shape { circle, box, polygon };
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
    // Convex vertices in counterclockwise order, relative to the center of mass.
    // Polygon geometry uses vertices; radius/half_extents remain circle/box fields.
    std::vector<Vec2> vertices;
};

struct Contact {
    Vec2 normal; // From the first body toward the second; correction moves them apart.
    Vec2 point;
    float penetration;
};

/// Conservative local extents and enclosing radius, including polygon geometry.
Vec2 local_extents(const Body &body);
float bounding_radius(const Body &body);
/// Regular centered convex polygon, 3..32 vertices; minimum edge length 0.0001.
/// Throws invalid_argument for invalid arguments or degenerate geometry.
Body regular_polygon(int sides, float radius);
Aabb bounds(const Body &body);
/// Detect circle, OBB and convex polygon contact, including touching shapes.
std::optional<Contact> collide(const Body &a, const Body &b);
/// Resolve an existing contact using the same impulses and correction as World::step.
void resolve_contact(Body &a, Body &b, const Contact &contact);
/// Apply a world-space impulse at a point, including angular momentum for dynamic bodies.
void apply_body_impulse(Body &body, Vec2 impulse, Vec2 point);
/// Semi-implicit integration; damping is a nonnegative linear drag rate per time unit.
void integrate(Body &body, float seconds, float damping = 0);
/// Earliest segment hit fraction, including a start inside the circle; nullopt on miss.
std::optional<float> segment_circle(Vec2 start, Vec2 end, Vec2 center, float radius);
struct SweepContact {
    float fraction;  // Earliest hit along the center segment, in [0, 1].
    Contact contact; // Normal from the moving body toward the obstacle.
};
/// Sweep a circle from its current position to end against an obstacle's current pose.
/// Includes initial overlap/touch (fraction zero); ignores obstacle velocity/acceleration.
/// Rejects noncircle movers and invalid geometry/endpoints with invalid_argument.
std::optional<SweepContact> sweep_circle(const Body &circle, Vec2 end, const Body &obstacle);
/// Sweep any supported convex body to end, holding both orientations fixed.
/// Obstacle motion and both angular velocities are ignored; initial contact is included.
/// Polygon contact reconstruction uses a scale-dependent floating-point tolerance;
/// throws runtime_error if a swept contact cannot be reconstructed numerically.
std::optional<SweepContact> sweep_translation(const Body &body, Vec2 end, const Body &obstacle);
/// Sweep two translating bodies to their explicit endpoints with fixed orientations.
/// The contact point is returned in world coordinates at the shared hit fraction.
std::optional<SweepContact> sweep_translation(const Body &a, Vec2 end_a, const Body &b, Vec2 end_b);
/// Validate finite state, positive shape/mass and material ranges; throws invalid_argument.
void validate(const Body &body);

/// Impulse simulation with a spatial broad phase. Body indices remain stable
/// until clear(); no removal API. Acceleration persists until changed by the caller.
/// step() accepts (0, 0.1] seconds; use a fixed timestep. Continuous sweeps cover
/// circles and nonrotating convex pairs, including moving/moving contacts.
/// Pairs containing rotating boxes/polygons remain discrete. Each substep has
/// 8 * max(1, moving body count) swept event iterations; exhausting the budget
/// discards remaining travel for all bodies, retaining velocities. Inspect
/// sweep_budget_exhausted() to detect this conservative fallback.
class World {
  public:
    explicit World(float cell_size = 4) : grid_(cell_size) {}
    std::size_t add(Body body);
    void clear() {
        bodies_.clear();
        candidate_pairs_ = 0;
        sweep_budget_exhausted_ = false;
    }
    Body &body(std::size_t id) { return bodies_.at(id); }
    const Body &body(std::size_t id) const { return bodies_.at(id); }
    std::span<const Body> bodies() const { return bodies_; }
    void step(float seconds);
    Vec2 gravity{};
    std::size_t candidate_pairs() const { return candidate_pairs_; }
    /// True if either substep discarded travel after reaching its swept-event budget.
    bool sweep_budget_exhausted() const { return sweep_budget_exhausted_; }

  private:
    std::vector<Body> bodies_;
    SpatialGrid grid_;
    std::size_t candidate_pairs_{};
    bool sweep_budget_exhausted_{};
};

} // namespace kinetra
