#pragma once

#include <cstdint>
#include <kinetra/physics.h>
#include <utility>
#include <vector>

namespace kinetra {

/// Bounded eight-neighbor A* over body geometry, with swept circular clearance.
/// The caller selects the envelope and grid resolution; world content stays external.
class Navigation {
  public:
    explicit Navigation(float resolution = 1, float radius = 0.85f);
    void rebuild(kinetra::Aabb bounds, const std::vector<kinetra::Body> &obstacles);

    /// Exact free goals are retained. Occupied goals resolve to the closest reachable
    /// free grid point (configured resolution); disconnected free goals still fail.
    std::vector<kinetra::Vec2> path(kinetra::Vec2 start, kinetra::Vec2 goal) const;

    /// Continuous swept-circle clearance, shared by planning and route following.
    bool clear(kinetra::Vec2 a, kinetra::Vec2 b) const;

  private:
    bool clear_body(const kinetra::Body &body, kinetra::Vec2 a, kinetra::Vec2 b) const;

    float spacing, clearance;
    kinetra::Aabb area{};
    int width{}, height{};
    // Opposite directions have complementary indices (7 - direction).
    static constexpr std::pair<int, int> offsets[]{{-1, -1}, {0, -1}, {1, -1}, {-1, 0},
                                                   {1, 0},   {-1, 1}, {0, 1},  {1, 1}};
    std::vector<bool> blocked;
    std::vector<std::uint8_t> links;
    std::vector<int> region;
    std::vector<kinetra::Body> geometry;
    kinetra::SpatialGrid index{4};

    /// Permit only outward departure from an inflated surface, never a shortcut
    /// through it. Convex distance cannot decrease along an outward segment.
    bool can_depart(kinetra::Vec2 start, kinetra::Vec2 end) const;

    int neighbor(int id, int direction) const;
    bool inside(kinetra::Vec2 p) const;
    kinetra::Vec2 point(int id) const;
};

/// Advance along visible segments using a lookahead target. Intermediate corners
/// are passed without arrival braking; the final endpoint still uses normal arrival.
kinetra::Vec2 route_target(kinetra::Vec2 position, std::vector<kinetra::Vec2> &route,
                           const Navigation &navigation);

} // namespace kinetra
