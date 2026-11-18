#include <iostream>
#include <kinetra/avoidance.h>
#include <stdexcept>
#include <vector>

using namespace kinetra;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void bounded_results() {
    AvoidanceAgent agent{0, {}, {}, 0.4f};
    AvoidanceSettings settings;
    settings.max_speed = 2;
    settings.max_acceleration = 1;
    auto result = avoid_collisions(agent, {2, 0}, {}, 0.1f, settings);
    require(result.feasible && length(result.acceleration) <= 1.00001f &&
                std::abs(result.velocity.x - 0.1f) < 0.00001f,
            "Free acceleration limit failed");
    agent.velocity = {10, 0};
    result = avoid_collisions(agent, {2, 0}, {}, 0.1f, settings);
    require(result.feasible && std::abs(result.velocity.x - 9.9f) < 0.00001f,
            "Overspeed must decelerate without instantaneous clamping");
    agent.velocity = {};
    std::vector<AvoidanceAgent> trapped{{1, {-0.1f, 0}, {}, 0.4f, false}, {2, {0.1f, 0}, {}, 0.4f, false}};
    result = avoid_collisions(agent, {2, 0}, trapped, 0.01f, settings);
    require(!result.feasible && result.violation > 0 && finite(result.velocity) &&
                length(result.acceleration) <= 1.00001f,
            "Infeasible avoidance must remain bounded and report failure");
    trapped.resize(1);
    trapped[0].position = {};
    result = avoid_collisions(agent, {}, trapped, 0.01f, settings);
    require(!result.feasible && result.velocity.x < 0,
            "Coincident agents must recover in a stable ID-based direction");
    settings.max_speed = 0;
    result = avoid_collisions(agent, {0.0001f, 0}, {}, 0.1f, settings);
    require(length(result.velocity) == 0, "Zero speed limit allowed motion");
    settings.max_speed = 2;
    settings.max_acceleration = 0;
    result = avoid_collisions(agent, {2, 0}, {}, 0.01f, settings);
    require(length(result.velocity) == 0 && length(result.acceleration) == 0,
            "Zero acceleration changed velocity");
}
void neighbor_order() {
    AvoidanceAgent agent{0, {}, {2, 0}, 0.4f};
    std::vector<AvoidanceAgent> neighbors{{1, {2, 0.1f}, {-1, 0}, 0.4f}, {2, {1, -1}, {0, 1}, 0.4f}};
    const auto a = avoid_collisions(agent, {2, 0}, neighbors, 0.1f);
    std::reverse(neighbors.begin(), neighbors.end());
    const auto b = avoid_collisions(agent, {2, 0}, neighbors, 0.1f);
    require(length(a.velocity - b.velocity) < 0.00001f && a.feasible == b.feasible,
            "Neighbor order changed avoidance");
    bool rejected = false;
    neighbors[0].id = neighbors[1].id;
    try {
        avoid_collisions(agent, {}, neighbors, 0.1f);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    require(rejected, "Duplicate neighbor IDs accepted");
    rejected = false;
    try {
        avoid_collisions(agent, {}, {}, 0);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    require(rejected, "Invalid avoidance timestep accepted");
}
void unilateral_crossing() {
    AvoidanceAgent a{0, {-6, 0}, {}, 0.4f}, b{1, {0, -6}, {0, 2}, 0.4f, false};
    AvoidanceSettings settings;
    settings.max_speed = 2;
    settings.max_acceleration = 8;
    settings.horizon = 2;
    float gap = 100;
    for (int tick = 0; tick < 1200; ++tick) {
        const Vec2 target{6, 0};
        const auto delta = target - a.position;
        const auto result = avoid_collisions(a, normalized(delta) * std::min(2.f, length(delta) * 2),
                                             std::span<const AvoidanceAgent>(&b, 1), 1.f / 60, settings);
        a.velocity = result.velocity;
        a.position += a.velocity / 60;
        b.position += b.velocity / 60;
        gap = std::min(gap, length(a.position - b.position) - a.radius - b.radius);
    }
    require(gap >= -0.001f && length(a.position - Vec2{6, 0}) < 0.3f,
            "Avoidance failed against a noncooperative moving neighbor");
}
void time_units(bool rectangles = false) {
    AvoidanceAgent a{0, {}, {2, 0}, 0.4f}, b{1, {3, 0.2f}, {-1, 0}, 0.4f};
    AvoidanceSettings settings;
    if (rectangles) {
        a.half_extents = {0.3f, 0.1f};
        b.half_extents = {0.2f, 0.3f};
    }
    const auto original = avoid_collisions(a, {2, 0}, std::span<const AvoidanceAgent>(&b, 1), 0.1f, settings);
    a.velocity = a.velocity / 60;
    b.velocity = b.velocity / 60;
    settings.max_speed /= 60;
    settings.max_acceleration /= 3600;
    settings.horizon *= 60;
    const auto ticks =
        avoid_collisions(a, {2.f / 60, 0}, std::span<const AvoidanceAgent>(&b, 1), 6, settings);
    require(length(ticks.velocity * 60 - original.velocity) < 0.001f &&
                length(ticks.acceleration * 3600 - original.acceleration) < 0.01f,
            "Consistent seconds/ticks conversion changed avoidance");
}

bool enters_rectangle(Vec2 travel, Vec2 center, Vec2 extent) {
    float enter = 0, leave = 1;
    for (int axis = 0; axis < 2; ++axis) {
        const float v = axis ? travel.y : travel.x;
        const float low = (axis ? center.y - extent.y : center.x - extent.x);
        const float high = (axis ? center.y + extent.y : center.x + extent.x);
        if (v == 0) {
            if (low > 0 || high < 0)
                return false;
        } else {
            enter = std::max(enter, std::min(low / v, high / v));
            leave = std::min(leave, std::max(low / v, high / v));
        }
    }
    return enter <= leave;
}
void rectangular_geometry() {
    AvoidanceAgent a{0, {}, {}, 1, true, {0.5f, 0.1f}};
    AvoidanceAgent b{1, {0, 0.25f}, {}, 1, false, {0.5f, 0.1f}};
    AvoidanceSettings settings;
    settings.max_speed = 3;
    settings.max_acceleration = 100;
    settings.horizon = 2;
    settings.clearance = 0.02f;
    settings.passing_bias = 0;
    auto result = avoid_collisions(a, {2, 0}, {&b, 1}, 1, settings);
    require(result.feasible && length(result.velocity - Vec2{2, 0}) < 0.00001f,
            "Rectangle avoidance blocked a valid gap inside the circular envelopes");
    // Independently check corrected velocities against the swept Minkowski box,
    // including all cutoff edges, tangent rays and corner projection regions.
    a.half_extents = {0.6f, 0.15f};
    b.half_extents = {0.2f, 0.7f};
    b.velocity = {0.3f, -0.7f};
    for (int direction = 0; direction < 24; ++direction) {
        b.position = rotated({3, 0}, static_cast<float>(direction) * 6.283185307f / 24);
        for (int heading = 0; heading < 24; ++heading) {
            a.velocity = rotated({2, 0}, static_cast<float>(heading) * 6.283185307f / 24);
            result = avoid_collisions(a, a.velocity, {&b, 1}, 1, settings);
            const auto inflated = a.half_extents + b.half_extents + Vec2{0.0199f, 0.0199f};
            require(result.feasible && !enters_rectangle((result.velocity - b.velocity) * settings.horizon,
                                                         b.position, inflated),
                    "Rectangle constraint admitted a finite-horizon collision");
        }
    }
    a.velocity = {};
    b.velocity = {};
    b.position = {};
    b.half_extents = a.half_extents;
    settings.max_acceleration = 0.1f;
    result = avoid_collisions(a, {}, {&b, 1}, 0.1f, settings);
    require(!result.feasible && finite(result.velocity) && length(result.acceleration) <= 0.10001f,
            "Overlapping rectangles hid infeasibility or exceeded acceleration limits");
    b.cooperative = true;
    for (Vec2 offset : {Vec2{}, Vec2{0.1f, 0}}) {
        b.position = offset;
        const auto first = avoid_collisions(a, {}, {&b, 1}, 0.1f, settings);
        const auto second = avoid_collisions(b, {}, {&a, 1}, 0.1f, settings);
        require(length(first.velocity + second.velocity) < 0.00001f && length(first.velocity) > 0,
                "Symmetric overlapping rectangles did not choose opposite recovery directions");
    }
    b.half_extents = {1, 0};
    bool rejected = false;
    try {
        avoid_collisions(a, {}, {&b, 1}, 1, settings);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    require(rejected, "Partial rectangle extents accepted");
    b.half_extents = {2, 2};
    rejected = false;
    try {
        avoid_collisions(a, {}, {&b, 1}, 1, settings);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    require(rejected, "Rectangle with an unsafe mixed-pair radius accepted");
}

void scenario(int count, bool parked, bool rectangles = false) {
    AvoidanceSettings settings;
    settings.max_speed = 2;
    settings.max_acceleration = 8;
    settings.horizon = 2;
    settings.clearance = 0.15f;
    std::vector<AvoidanceAgent> agents;
    std::vector<Vec2> goals;
    for (int i = 0; i < count; ++i) {
        const Vec2 position =
            rotated({-6, 0}, 6.283185307f * static_cast<float>(i) / static_cast<float>(count));
        agents.push_back({std::uint64_t(i), position, {}, 0.4f});
        goals.push_back(-position);
    }
    if (parked) {
        agents.resize(1);
        goals.resize(1);
        agents.push_back({1, {}, {}, 1, false});
        goals.push_back({});
    }
    if (rectangles)
        for (auto &agent : agents) {
            agent.half_extents = {0.35f, 0.15f};
            agent.radius = length(agent.half_extents);
        }
    float minimum_gap = 100;
    for (int tick = 0; tick < 1200; ++tick) {
        const auto snapshot = agents;
        for (std::size_t i = 0; i < agents.size(); ++i) {
            if (!agents[i].cooperative)
                continue;
            std::vector<AvoidanceAgent> neighbors;
            for (std::size_t j = 0; j < snapshot.size(); ++j)
                if (i != j)
                    neighbors.push_back(snapshot[j]);
            const auto delta = goals[i] - agents[i].position;
            const auto preferred = normalized(delta) * std::min(settings.max_speed, length(delta) * 2);
            const auto result = avoid_collisions(snapshot[i], preferred, neighbors, 1.f / 60, settings);
            require(length(result.acceleration) <= settings.max_acceleration + 0.001f &&
                        length(result.velocity) <= settings.max_speed + 0.001f,
                    "Scenario violated motion limits");
            agents[i].velocity = result.velocity;
            agents[i].position += result.velocity / 60;
        }
        for (std::size_t i = 0; i < agents.size(); ++i)
            for (std::size_t j = i + 1; j < agents.size(); ++j)
                if (rectangles) {
                    const auto delta = agents[i].position - agents[j].position;
                    const auto extent = agents[i].half_extents + agents[j].half_extents;
                    minimum_gap = std::min(
                        minimum_gap, std::max(std::abs(delta.x) - extent.x, std::abs(delta.y) - extent.y));
                } else {
                    minimum_gap = std::min(minimum_gap, length(agents[i].position - agents[j].position) -
                                                            agents[i].radius - agents[j].radius);
                }
    }
    std::cout << "scenario " << count << " parked " << parked << " min gap " << minimum_gap << '\n';
    require(minimum_gap >= -0.001f, "Predictive avoidance allowed physical overlap");
    for (std::size_t i = 0; i < agents.size(); ++i)
        if (agents[i].cooperative)
            require(length(agents[i].position - goals[i]) < 0.3f && length(agents[i].velocity) < 0.05f,
                    "Avoidance scenario failed to arrive and stop at goal");
}
} // namespace
int main() {
    try {
        rectangular_geometry();
        time_units(true);
        scenario(2, false, true);
        scenario(4, false, true);
        scenario(8, false, true);
        bounded_results();
        neighbor_order();
        unilateral_crossing();
        time_units();
        scenario(2, false);
        scenario(4, false);
        scenario(2, true);
        scenario(8, false);
        scenario(16, false);
        std::cout << "Avoidance contracts passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
