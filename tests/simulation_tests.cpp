#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <kinetra/camera.h>
#include <kinetra/collision_grid.h>
#include <kinetra/steering.h>
#include <kinetra/timing.h>
#include <limits>
#include <random>
#include <stdexcept>

using namespace kinetra;

namespace {

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
void near(float a, float b, float tolerance = 0.001f) {
    require(std::abs(a - b) <= tolerance, "Values differ");
}
void rejects(const std::function<void()> &action) {
    try {
        action();
    } catch (const std::invalid_argument &) {
        return;
    }
    throw std::runtime_error("Invalid input was accepted");
}
Body circle(Vec2 p, float radius = 0.5f) {
    Body b;
    b.position = p;
    b.radius = radius;
    return b;
}
Body box(Vec2 p, Vec2 size, float angle = 0) {
    Body b;
    b.shape = Shape::box;
    b.position = p;
    b.half_extents = size;
    b.angle = angle;
    return b;
}

void collision_geometry() {
    auto a = circle({0, 0}), b = circle({1, 0});
    auto c = collide(a, b);
    require(c.has_value(), "Touching circles missed");
    near(c->penetration, 0);
    near(c->normal.x, 1);
    b.position.x = 1.1f;
    require(!collide(a, b), "Separated circles collide");
    b.position = a.position;
    c = collide(a, b);
    require(c && finite(c->normal), "Coincident circles invalid");
    const Body rotated_box = box({0, 0}, {2, 0.25f}, 0.78539816f);
    require(collide(rotated_box, circle({1.2f, 1.2f})).has_value(), "Rotated box missed");
    require(!collide(rotated_box, circle({1.2f, -1.2f}, 0.2f)), "Rotated box false positive");
    auto inside = collide(circle({0, 0}), box({0, 0}, {2, 1}));
    require(inside && inside->penetration > 1, "Containment not resolved");
    const Body corner = circle({2.3f, 1.3f});
    c = collide(corner, box({0, 0}, {2, 1}));
    require(c && c->normal.x < 0 && c->normal.y < 0, "Corner normal incorrect");
    auto reverse = collide(box({0, 0}, {2, 1}), corner);
    near(dot(c->normal, reverse->normal), -1);
    require(collide(rotated_box, box({0.5f, 0.5f}, {0.5f, 0.5f}, -0.3f)).has_value(), "OBB contact missed");
    require(!collide(rotated_box, box({4, -3}, {0.5f, 0.5f}, -0.3f)), "OBB false positive");
}

void grid_matches_exhaustive_search() {
    std::mt19937 rng(17);
    std::uniform_real_distribution<float> coordinate(-40, 40), size(0.01f, 5);
    std::vector<Aabb> boxes;
    for (int i = 0; i < 400; ++i) {
        const Vec2 center{coordinate(rng), coordinate(rng)}, extent{size(rng), size(rng)};
        boxes.push_back({center - extent, center + extent});
    }
    SpatialGrid grid(3);
    grid.rebuild(boxes);
    for (const auto &area : boxes) {
        std::vector<std::size_t> expected;
        for (std::size_t j = 0; j < boxes.size(); ++j)
            if (boxes[j].overlaps(area))
                expected.push_back(j);
        require(grid.query(area) == expected, "Spatial index differs from exhaustive results");
    }
    grid.rebuild({});
    require(grid.query({{-1, -1}, {1, 1}}).empty(), "Rebuild retained stale objects");
}

void elastic_momentum() {
    World world;
    Body a = circle({-0.48f, 0}), b = circle({0.48f, 0});
    a.velocity = {2, 0};
    b.velocity = {-1, 0};
    a.restitution = b.restitution = 1;
    a.friction = b.friction = 0;
    const auto ai = world.add(a), bi = world.add(b);
    world.step(1.f / 120);
    near(world.body(ai).velocity.x, -1);
    near(world.body(bi).velocity.x, 2);
    near(world.body(ai).velocity.x + world.body(bi).velocity.x, 1);
    near(dot(world.body(ai).velocity, world.body(ai).velocity) +
             dot(world.body(bi).velocity, world.body(bi).velocity),
         5);
    require(length(world.body(ai).position - world.body(bi).position) > 0.99f, "Penetration remains");
}

void controlled_wall_slide() {
    World world;
    Body wall = box({0, 0}, {0.5f, 10});
    wall.motion = Motion::fixed;
    Body mover = circle({-1.1f, 0});
    mover.motion = Motion::controlled;
    mover.velocity = {3, 2};
    mover.friction = wall.friction = 0;
    const auto wi = world.add(wall), id = world.add(mover);
    for (int i = 0; i < 60; ++i)
        world.step(1.f / 120);
    near(world.body(id).velocity.x, 0);
    near(world.body(id).velocity.y, 2);
    near(world.body(id).angular_velocity, 0);
    require(world.body(id).position.x < -0.99f && world.body(id).position.y > 0.9f,
            "Controlled body did not slide");
    near(world.body(wi).position.x, 0);
}

void collision_rotates_dynamic_only() {
    for (Motion motion : {Motion::dynamic, Motion::controlled}) {
        World world;
        Body obstacle = box({0, 0}, {0.5f, 0.5f});
        obstacle.motion = Motion::fixed;
        Body mover = box({-1.05f, 0.65f}, {0.6f, 0.5f});
        mover.velocity = {4, 0};
        mover.motion = motion;
        mover.friction = obstacle.friction = 0;
        world.add(obstacle);
        const auto id = world.add(mover);
        world.step(1.f / 120);
        if (motion == Motion::dynamic)
            require(std::abs(world.body(id).angular_velocity) > 0.1f,
                    "Off-center collision produced no torque");
        else
            near(world.body(id).angular_velocity, 0);
    }
}

void mixed_modes_and_repeatability() {
    World world;
    Body controlled = circle({-0.49f, 0});
    controlled.motion = Motion::controlled;
    controlled.velocity = {4, 0};
    Body physical = circle({0.49f, 0});
    controlled.friction = physical.friction = 0;
    const auto first = world.add(controlled), second = world.add(physical);
    World replay = world;
    for (int i = 0; i < 120; ++i) {
        world.step(1.f / 120);
        replay.step(1.f / 120);
    }
    require(world.body(second).velocity.x > 1, "Controlled body failed to push dynamic body");
    require(world.body(first).velocity.x >= 0, "Controlled collision bounced");
    near(world.body(first).position.x, replay.body(first).position.x, 0);
    near(world.body(second).position.x, replay.body(second).position.x, 0);
    world.clear();
    require(world.bodies().empty() && world.candidate_pairs() == 0, "World clear retained state");
}

void gravity_floor_and_friction() {
    World world;
    world.gravity = {0, -10};
    Body floor = box({0, -1}, {20, 0.5f});
    floor.motion = Motion::fixed;
    floor.friction = 1;
    Body resting = box({0, 2}, {0.5f, 0.5f});
    resting.friction = 1;
    resting.restitution = 0;
    resting.velocity = {3, 0};
    world.add(floor);
    const auto id = world.add(resting);
    for (int i = 0; i < 1200; ++i)
        world.step(1.f / 120);
    const auto &result = world.body(id);
    require(finite(result.position) && result.position.y > -0.03f && result.position.y < 0.1f,
            "Body did not rest on floor");
    require(std::abs(result.velocity.x) < 0.1f && std::abs(result.velocity.y) < 0.1f,
            "Friction/resting contact unstable");
}

void gravity_and_containment() {
    World world;
    world.gravity = {0, -10};
    const auto id = world.add(circle({0, 0}));
    for (int i = 0; i < 120; ++i)
        world.step(1.f / 120);
    near(world.body(id).velocity.y, -10, 0.001f);
    near(world.body(id).position.y, -5, 0.03f);
    World inside;
    Body container = box({0, 0}, {2, 1});
    container.motion = Motion::fixed;
    inside.add(container);
    const auto ci = inside.add(circle({0, 0}));
    for (int i = 0; i < 4; ++i)
        inside.step(1.f / 120);
    require(std::abs(inside.body(ci).position.y) > 1.49f, "Contained circle failed to exit");
}

void steering_arrives_and_avoids() {
    Body agent = circle({0, 0});
    Steering settings;
    Vec2 target{10, 0};
    for (int i = 0; i < 1200; ++i) {
        const Vec2 acceleration = steer(agent, target, {}, settings);
        require(length(acceleration) <= settings.max_acceleration + 0.001f, "Unbounded steering");
        agent.velocity += acceleration * (1.f / 120);
        agent.position += agent.velocity * (1.f / 120);
    }
    require(length(agent.position - target) < 0.2f && length(agent.velocity) < 0.05f,
            "Arrival failed to brake");
    agent = circle({0, 0});
    agent.velocity = {3, 0};
    const std::array<Body, 1> obstacles{box({2, 0}, {0.4f, 1}, 0.3f)};
    const Vec2 avoided = steer(agent, target, obstacles, settings);
    require(std::abs(avoided.y) > 0.1f && avoided.x < 0, "Obstacle did not change steering");
    // Integrated avoidance must pass a single obstacle without relying on collision correction.
    agent = circle({-5, 0}, 0.3f);
    target = {5, 0};
    const std::array<Body, 1> barrier{box({0, 0}, {0.4f, 1})};
    for (int i = 0; i < 2400; ++i) {
        agent.velocity += steer(agent, target, barrier, settings) * (1.f / 120);
        agent.position += agent.velocity * (1.f / 120);
        const auto c = collide(agent, barrier[0]);
        require(!c || c->penetration < 0.01f, "Steering ran into obstacle");
    }
    require(length(agent.position - target) < 0.3f, "Steering did not get around obstacle");
    // At rest, personal space still pushes away; it vanishes beyond the radius.
    agent = circle({0, 0}, 0.2f);
    const std::array<Body, 1> neighbor{circle({0, 1}, 0.2f)};
    settings.separation_radius = 2;
    const auto separated = steer(agent, agent.position, neighbor, settings);
    require(separated.y < 0 && length(separated) <= settings.max_acceleration,
            "Separation must push away with bounded acceleration");
    settings.separation_radius = 0.5f;
    require(length(steer(agent, agent.position, neighbor, settings)) == 0,
            "Distant neighbors must not cause separation");
    settings.separation_radius = -1;
    rejects([&] { (void)steer(agent, agent.position, neighbor, settings); });
}

void timing_and_camera() {
    FixedClock a(0.01, 16), b(0.01, 16);
    int ticks_a = 0, ticks_b = 0;
    for (int i = 0; i < 100; ++i)
        ticks_a += a.advance(0.01);
    for (int i = 0; i < 200; ++i)
        ticks_b += b.advance(0.005);
    require(ticks_a == 100 && ticks_b == 100, "Clock depends on rendering frequency");
    require(a.advance(10) == 16 && a.interpolation() < 1, "Backlog not bounded");
    Camera2D camera;
    camera.position = {4, -3};
    camera.viewport = {1920, 1080};
    camera.pixels_per_unit = 32;
    const Vec2 point{-17, 11};
    const Vec2 back = camera.screen_to_world(camera.world_to_screen(point));
    near(back.x, point.x);
    near(back.y, point.y);
    near(camera.world_to_screen(camera.position).x, 960);
}

void invalid_input() {
    rejects([] { SpatialGrid grid(0); });
    SpatialGrid grid;
    rejects([&] { (void)grid.query({{2, 0}, {1, 1}}); });
    rejects([&] { (void)grid.query({{-1e30f, 0}, {1e30f, 1}}); });
    World world;
    rejects([&] { world.step(0); });
    rejects([&] { world.step(1); });
    Body b;
    b.mass = 0;
    rejects([&] { world.add(b); });
    b = Body{};
    b.position.x = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { world.add(b); });
    Steering s;
    s.response_time = 0;
    rejects([&] { (void)steer(Body{}, {}, {}, s); });
    rejects([] { FixedClock clock(-1); });
    Camera2D camera;
    camera.pixels_per_unit = 0;
    rejects([&] { (void)camera.world_to_screen({}); });
}

void hundreds_of_agents() {
    World world;
    for (int i = 0; i < 400; ++i) {
        Body b = circle({float(i % 20) * 1.8f, float(i / 20) * 1.8f}, 0.3f);
        if (i % 3 == 0) {
            b.shape = Shape::box;
            b.half_extents = {0.3f, 0.2f};
            b.angle = i * 0.1f;
        }
        b.motion = i % 2 ? Motion::controlled : Motion::dynamic;
        world.add(b);
    }
    const auto start = std::chrono::steady_clock::now();
    SpatialGrid neighbors(4);
    std::vector<Aabb> boxes;
    std::vector<Body> nearby;
    for (int tick = 0; tick < 240; ++tick) {
        boxes.clear();
        for (const auto &b : world.bodies())
            boxes.push_back(bounds(b));
        neighbors.rebuild(boxes);
        for (std::size_t i = 0; i < world.bodies().size(); ++i) {
            auto &b = world.body(i);
            nearby.clear();
            for (auto j : neighbors.query({b.position - Vec2{6, 6}, b.position + Vec2{6, 6}}))
                if (j != i)
                    nearby.push_back(world.body(j));
            b.acceleration = steer(b, {18, 18}, nearby);
        }
        world.step(1.f / 120);
        for (const auto &b : world.bodies()) {
            require(finite(b.position) && finite(b.velocity) && std::isfinite(b.angle),
                    "Nonfinite swarm state");
            require(length(b.velocity) < 20, "Unstable swarm velocity");
        }
    }
    const double ms =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << "  400 bodies, 240 steps including steering: " << ms << " ms (" << ms / 240 << " ms/step)\n";
}

void reusable_game_physics() {
    const auto tiny_hit = segment_circle({1.0001f, 0}, {0.9999f, 0}, {}, 1);
    require(tiny_hit && *tiny_hit > 0 && *tiny_hit < 1,
            "Tiny relative projectile travel tunneled through circle");
    require(segment_circle({-10000, 25}, {10000, 25}, {}, 25).has_value(),
            "Long grazing projectile missed tangent contact");
    // Insertion order must not drop moving/fixed pairs when fixed queries are skipped.
    for (bool fixed_first : {false, true}) {
        World world;
        Body wall, mover;
        wall.motion = Motion::fixed;
        wall.position = {0, 0};
        mover.position = {-0.9f, 0};
        mover.velocity = {1, 0};
        const auto a = world.add(fixed_first ? wall : mover);
        const auto b = world.add(fixed_first ? mover : wall);
        world.step(1.f / 120);
        require(world.body(fixed_first ? b : a).position.x < -0.95f,
                "Fixed/moving contact depends on insertion order");
    }

    near(*segment_circle({0, -2}, {0, 2}, {}, 1), 0.25f);
    require(!segment_circle({2, -2}, {2, 2}, {}, 1), "Circle sweep false positive");
    near(*segment_circle({}, {}, {}, 1), 0);
    require(!segment_circle({2, 0}, {2, 0}, {}, 1), "Stationary outside segment hit");
    rejects([] { segment_circle({}, {}, {}, -1); });
    const std::array<int, 9> cells{0, 0, 0, 0, 1, 0, 0, 0, 0};
    CollisionGrid grid{cells, 3, 3, 2, {-3, -3}};
    near(*segment_grid({-2, 0}, {2, 0}, grid), 0.25f);
    require(!segment_grid({-2, -2}, {2, -2}, grid), "Clear grid ray hit");
    near(*segment_grid({-2, -2}, {2, 2}, grid), 0.25f);
    near(*segment_grid({-2, -2}, {-4, -2}, grid), 0.5f);
    require(!segment_grid({-2, -2}, {-2, -2}, grid), "Stationary empty grid ray hit");
    Body actor;
    actor.motion = Motion::controlled;
    actor.position = {-1.3f, 0};
    actor.velocity = {2, 0};
    actor.radius = 0.5f;
    collide_grid(actor, grid);
    require(actor.position.x < -1.49f && actor.velocity.x == 0, "Grid collision failed to stop actor");
    Body crate;
    actor.position = {-0.8f, 0};
    actor.velocity = {3, 0};
    crate.position = {};
    crate.radius = 0.5f;
    crate.mass = 0.5f;
    resolve_contact(actor, crate, *collide(actor, crate));
    require(crate.velocity.x > 0 && actor.velocity.x < 3, "Contact failed to transfer momentum");
    const auto old = crate.position;
    integrate(crate, 0.5f, 0.2f);
    require(crate.position.x > old.x && crate.velocity.x > 0, "Crate did not coast after pushing");
    Body spinning;
    apply_body_impulse(spinning, {2, 0}, {0, 1});
    require(spinning.velocity.x == 2 && spinning.angular_velocity < 0, "Off-center impulse omitted torque");
    spinning.motion = Motion::fixed;
    const auto velocity = spinning.velocity;
    apply_body_impulse(spinning, {2, 0}, {});
    near(spinning.velocity.x, velocity.x);
    rejects([&] { integrate(crate, -1); });
    rejects([&] { segment_grid({}, {}, {{}, 2, 2, 1, {}}); });
}

} // namespace

int main() {
    const std::array<std::pair<const char *, void (*)()>, 13> cases{
        {{"reusable game physics", reusable_game_physics},
         {"collision geometry", collision_geometry},
         {"spatial queries", grid_matches_exhaustive_search},
         {"elastic momentum", elastic_momentum},
         {"controlled wall slide", controlled_wall_slide},
         {"angular collision response", collision_rotates_dynamic_only},
         {"mixed modes and repeatability", mixed_modes_and_repeatability},
         {"resting contact and friction", gravity_floor_and_friction},
         {"gravity and containment", gravity_and_containment},
         {"steering arrival and avoidance", steering_arrives_and_avoids},
         {"timing and camera", timing_and_camera},
         {"invalid inputs", invalid_input},
         {"hundreds of agents", hundreds_of_agents}}};
    int failures = 0;
    for (const auto &[name, run] : cases) {
        try {
            run();
            std::cout << "PASS " << name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    }
    return failures ? 1 : 0;
}
