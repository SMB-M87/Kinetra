#include <kinetra/camera.h>
#include <kinetra/avoidance.h>
#include <kinetra/navigation.h>
#include <kinetra/steering.h>
#include <kinetra/timing.h>
#include <kinetra/traffic.h>
#include <kinetra/weighted_grid.h>

#ifdef CONSUMER_GRAPHICS
#include <kinetra/launcher.h>
#include <kinetra/session.h>
auto consumer_run_launcher = &kinetra::run_launcher;
// Retain a relocation to a compiled Runtime entry point without opening a window.
auto consumer_run_session = &kinetra::run_session;
#endif

int main() {
    kinetra::TrafficGate crossing;
    const std::uint64_t requester = 4;
    if (crossing.update({&requester, 1}, {}).agent != requester) return 17;
    crossing.clear();
    if (crossing.update({&requester, 1}, {}, {&requester, 1}).agent) return 18;
    kinetra::AvoidanceAgent agent;
    const auto avoidance = kinetra::avoid_collisions(agent, {1, 0}, {}, 1.f / 60);
    if (!avoidance.feasible || !kinetra::finite(avoidance.acceleration)) return 15;
    agent.half_extents = {0.3f, 0.2f};
    kinetra::AvoidanceAgent parked{1, {0, 0.5f}, {}, 0.5f, false, {0.3f, 0.2f}};
    const auto rectangle_avoidance = kinetra::avoid_collisions(agent, {1, 0}, {&parked, 1}, 1.f / 60);
    if (!rectangle_avoidance.feasible) return 16;
    auto polygon = kinetra::regular_polygon(5, 1);
    if (kinetra::bounding_radius(polygon) < 0.99f) return 10;
#ifdef CONSUMER_GRAPHICS
    // Public session and input types remain independent of Lab.
    kinetra::LaunchMenu menu({{"CONSUMER", "External application", {}, 0}});
    if (menu.row_count() != 1)
        return 3;
    kinetra::Input input;
    if (input.held[std::size_t(kinetra::Key::enter)])
        return 2;
#endif
    kinetra::WeightedGrid grid({30, 30}, {10, 10});
    if (grid.path({5, 5}, {25, 25}).empty())
        return 4;
    if (grid.path({5, 5}, {25, 25}, {1, 1}).empty() ||
        !grid.clear({5, 5}, {25, 25}, {1, 1}))
        return 6;
    kinetra::Body rectangle;
    rectangle.shape = kinetra::Shape::box;
    if (!kinetra::finite(kinetra::priority_steer(rectangle, {10, 0}, {}, {}).acceleration))
        return 5;
    if (!kinetra::finite(kinetra::constrain_borders(rectangle, {}, {})))
        return 7;
    kinetra::Body circle;
    circle.position = {-3, 0};
    if (!kinetra::sweep_circle(circle, {3, 0}, rectangle))
        return 11;
    rectangle.position = {-3, 0};
    if (!kinetra::sweep_translation(rectangle, {3, 0}, polygon))
        return 12;
    circle.position = {3, 0};
    if (!kinetra::sweep_translation(rectangle, {3, 0}, circle, {-3, 0}))
        return 13;
    kinetra::World world;
    world.add({});
    world.step(1.f / 120);
    if (world.sweep_budget_exhausted()) return 14;
    kinetra::Navigation navigation;
    navigation.rebuild({{-2, -2}, {2, 2}}, {});
    return navigation.path({}, {1, 1}).empty() ? 1 : 0;
}
