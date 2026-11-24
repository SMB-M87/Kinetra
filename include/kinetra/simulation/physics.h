#pragma once

#include <kinetra/simulation/spatial_grid.h>
#include <optional>

namespace kinetra {

/// Collision geometry of a Body: Body::radius, Body::half_extents or Body::vertices.
enum class Shape { circle, box, polygon };
/// How a Body responds to integration and contacts.
///
/// `fixed` never moves and has infinite mass. `controlled` integrates and is pushed apart
/// by contacts, but receives no collision torque or restitution, so the caller keeps
/// control of its velocity. `dynamic` receives linear and angular impulses.
enum class Motion { fixed, controlled, dynamic };

/// Rigid body state, in world units, seconds and radians; positive angles rotate
/// counterclockwise.
///
/// A value type: World stores copies, and the free functions below take bodies by reference
/// without keeping them. validate() states the accepted ranges; the functions below validate
/// their bodies and throw std::invalid_argument, except local_extents(), bounding_radius()
/// and bounds().
struct Body {
    /// Which geometry field applies.
    Shape shape{Shape::circle};
    /// Response to integration and contacts.
    Motion motion{Motion::dynamic};
    /// Center of mass in world units, linear velocity in units per second, and acceleration
    /// in units per second squared. Acceleration persists until the caller changes it.
    Vec2 position{}, velocity{}, acceleration{};
    /// Half width and half height of a box, before rotation. Ignored for other shapes, but
    /// must still be positive.
    Vec2 half_extents{0.5f, 0.5f};
    /// `radius`: circle radius in world units; ignored but positive for other shapes.
    /// `angle`: orientation in radians. `angular_velocity`: radians per second.
    float radius{0.5f}, angle{}, angular_velocity{};
    /// `mass`: positive. `restitution` (bounciness) and `friction`: both in [0, 1].
    float mass{1}, restitution{0.4f}, friction{0.3f};
    /// Polygon outline: 3 to 32 strictly convex vertices in counterclockwise order, relative
    /// to the center of mass. Used only when `shape` is Shape::polygon.
    std::vector<Vec2> vertices;
};

/// Result of a contact test between two bodies, in world coordinates.
struct Contact {
    /// Unit normal from the first body toward the second; correction moves them apart.
    Vec2 normal;
    /// Representative contact point.
    Vec2 point;
    /// Overlap depth along `normal`; zero for touching shapes.
    float penetration;
};

/// Conservative half extents of the body in its local (unrotated) frame.
/// Does not validate the body.
Vec2 local_extents(const Body &body);
/// Radius of a circle around the center that encloses the whole shape.
/// Does not validate the body.
float bounding_radius(const Body &body);
/// Regular centered convex polygon with 3..32 vertices on a circle of `radius` world units.
/// Throws std::invalid_argument for other side counts, a radius below 0.0001 or degenerate
/// geometry. The other Body fields keep their defaults.
Body regular_polygon(int sides, float radius);
/// World-space axis-aligned bounds of the body at its current position and angle.
/// Does not validate the body.
Aabb bounds(const Body &body);
/// Detects circle, oriented box and convex polygon contact, including touching shapes.
/// Returns std::nullopt when the shapes are apart. Validates both bodies.
std::optional<Contact> collide(const Body &a, const Body &b);
/// Resolves an existing contact using the same impulses and correction as World::step.
/// Throws std::invalid_argument for invalid bodies, a non-unit normal or a negative or
/// nonfinite penetration.
void resolve_contact(Body &a, Body &b, const Contact &contact);
/// Applies a world-space impulse at a world-space point. Dynamic bodies also gain angular
/// momentum; fixed bodies do not move. Throws std::invalid_argument for nonfinite input.
void apply_body_impulse(Body &body, Vec2 impulse, Vec2 point);
/// Semi-implicit Euler integration over `seconds`: acceleration updates velocity, then
/// velocity updates position and angle. `damping` is a nonnegative linear drag rate per
/// second. Fixed bodies are left unchanged. Throws std::invalid_argument for negative or
/// nonfinite time or damping.
void integrate(Body &body, float seconds, float damping = 0);
/// Earliest fraction in [0, 1] at which the segment from `start` to `end` touches the
/// circle; zero when `start` is inside or on it, std::nullopt on a miss. Throws
/// std::invalid_argument for nonfinite input or a negative radius.
std::optional<float> segment_circle(Vec2 start, Vec2 end, Vec2 center, float radius);
/// Result of a sweep query.
struct SweepContact {
    /// Earliest hit along the swept center segment, in [0, 1].
    float fraction;
    /// Contact at that fraction; the normal points from the moving body toward the obstacle.
    Contact contact;
};
/// Sweeps a circle from its current position to `end` against an obstacle's current pose.
/// Includes initial overlap or touch (fraction zero); ignores obstacle velocity and
/// acceleration. Throws std::invalid_argument for a noncircle mover, invalid bodies or a
/// nonfinite endpoint.
std::optional<SweepContact> sweep_circle(const Body &circle, Vec2 end, const Body &obstacle);
/// Sweeps any supported convex body to `end`, holding both orientations fixed.
/// Obstacle motion and both angular velocities are ignored; initial contact is included.
/// Polygon contact reconstruction uses a scale-dependent floating-point tolerance;
/// throws std::runtime_error if a swept contact cannot be reconstructed numerically.
std::optional<SweepContact> sweep_translation(const Body &body, Vec2 end, const Body &obstacle);
/// Sweeps two translating bodies to their explicit endpoints with fixed orientations.
/// The contact point is returned in world coordinates at the shared hit fraction.
/// Motion modes do not override the endpoints.
std::optional<SweepContact> sweep_translation(const Body &a, Vec2 end_a, const Body &b, Vec2 end_b);
/// Throws std::invalid_argument unless the state is finite, mass and dimensions are at
/// least 0.0001, restitution and friction lie in [0, 1], and a polygon has 3..32 strictly
/// convex counterclockwise vertices centered on its area centroid.
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
    /// `cell_size` is the broad-phase grid cell in world units; it must be finite and
    /// positive (std::invalid_argument otherwise).
    explicit World(float cell_size = 4) : grid_(cell_size), fixed_grid_(cell_size) {}
    /// Validates and stores a copy of `body`; returns its index. Throws
    /// std::invalid_argument for an invalid body.
    std::size_t add(Body body);
    /// Removes all bodies and resets the step statistics. Previous indices become invalid.
    void clear() {
        bodies_.clear();
        candidate_pairs_ = 0;
        sweep_budget_exhausted_ = false;
    }
    /// Stored body by index; throws std::out_of_range for an unknown index. The reference
    /// stays valid until the next add() or clear(). Changes take effect on the next step().
    Body &body(std::size_t id) { return bodies_.at(id); }
    /// Read-only stored body by index; throws std::out_of_range for an unknown index.
    const Body &body(std::size_t id) const { return bodies_.at(id); }
    /// All stored bodies in index order; valid until the next add() or clear().
    std::span<const Body> bodies() const { return bodies_; }
    /// Advances all bodies by `seconds` in two substeps: forces, swept contacts on a shared
    /// timeline, then six discrete correction passes. Throws std::invalid_argument for a
    /// step outside (0, 0.1], nonfinite gravity or an invalid stored body.
    void step(float seconds);
    /// Gravity in world units per second squared, applied to every nonfixed body.
    Vec2 gravity{};
    /// Number of narrow-phase pair checks (swept and discrete) during the last step(); a
    /// cost diagnostic, not a count of distinct pairs.
    std::size_t candidate_pairs() const { return candidate_pairs_; }
    /// True if either substep discarded travel after reaching its swept-event budget.
    bool sweep_budget_exhausted() const { return sweep_budget_exhausted_; }

  private:
    bool fixed_index_current() const;
    void index_fixed_bodies();
    void candidates(Aabb area, bool with_fixed, std::vector<std::size_t> &out);

    std::vector<Body> bodies_;
    // Broad phase of step(). Fixed bodies never move during a step, so they have their own
    // index, rebuilt only when one of them differs from `fixed_bodies_`, the copies it was
    // built from; `grid_` indexes the moving bodies again for every pass. `fixed_ids_` and
    // `moving_ids_` give the body index of each grid index, ascending; `boxes_` holds the box
    // of every body in the current pass.
    SpatialGrid grid_;
    SpatialGrid fixed_grid_;
    std::vector<Body> fixed_bodies_;
    std::vector<std::size_t> fixed_ids_, moving_ids_;
    std::vector<Aabb> boxes_, moving_boxes_;
    std::vector<std::size_t> fixed_found_, moving_found_;
    bool fixed_index_valid_{};
    std::size_t candidate_pairs_{};
    bool sweep_budget_exhausted_{};
};

} // namespace kinetra
