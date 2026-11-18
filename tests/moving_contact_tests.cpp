#include <kinetra/physics.h>
#include <iostream>
#include <stdexcept>

using namespace kinetra;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
Body agent(int shape, Vec2 position, Vec2 velocity) {
    Body body;
    if (shape == 2)
        body = regular_polygon(3, 0.2f);
    else
        body.shape = shape == 0 ? Shape::circle : Shape::box;
    body.radius = 0.2f;
    body.half_extents = {0.2f, 0.3f};
    body.position = position;
    body.velocity = velocity;
    body.restitution = 1;
    body.friction = 0;
    return body;
}
void pair_query() {
    const auto a = agent(0, {-2, 0}, {}), b = agent(0, {2, 0}, {});
    const auto hit = sweep_translation(a, {2, 0}, b, {-2, 0});
    require(hit && std::abs(hit->fraction - 0.45f) < 0.00001f &&
                length(hit->contact.point) < 0.0001f &&
                length(hit->contact.normal - Vec2{1, 0}) < 0.0001f,
            "Moving pair sweep has incorrect time/point/normal");
    const Vec2 drift{3, -4};
    const auto drifting = sweep_translation(a, Vec2{2, 0} + drift, b, Vec2{-2, 0} + drift);
    require(drifting && std::abs(drifting->fraction - hit->fraction) < 0.00001f &&
                length(drifting->contact.point - (hit->contact.point + drift * hit->fraction)) < 0.0001f,
            "Pair sweep failed to map relative contact into world coordinates");
    const auto reversed = sweep_translation(b, {-2, 0}, a, {2, 0});
    require(reversed && std::abs(reversed->fraction - hit->fraction) < 0.00001f &&
                length(reversed->contact.normal + hit->contact.normal) < 0.0001f,
            "Pair sweep changes under reversed argument order");
    require(!sweep_translation(a, Vec2{2, 0}, b, Vec2{6, 0}), "Co-moving separated pair collided");
}
void fast_head_on() {
    for (int shape_a = 0; shape_a < 3; ++shape_a)
        for (int shape_b = 0; shape_b < 3; ++shape_b) {
            World world;
            auto a = agent(shape_a, {-2, 0}, {600, 0});
            auto b = agent(shape_b, {2, 0}, {-600, 0});
            a.motion = b.motion = Motion::controlled;
            world.add(a); world.add(b);
            world.step(1.f / 60);
            require(world.body(0).position.x < world.body(1).position.x,
                    "Fast controlled agents exchanged sides without contact");
            const auto contact = collide(world.body(0), world.body(1));
            require(!contact || contact->penetration < 0.001f, "Controlled pair remained deeply overlapped");
            require(!world.sweep_budget_exhausted(), "Single pair exhausted swept-event budget");
        }
    for (int shape = 0; shape < 2; ++shape) {
        World world;
        world.add(agent(shape, {-2, 0}, {600, 0}));
        world.add(agent(shape, {2, 0}, {-600, 0}));
        world.step(1.f / 60);
        const auto &a = world.body(0), &b = world.body(1);
        require(std::abs(a.velocity.x + 600) < 0.001f && std::abs(b.velocity.x - 600) < 0.001f,
                "Elastic moving pair did not exchange velocities");
        require(std::abs(a.position.x + 8.4f) < 0.001f && std::abs(b.position.x - 8.4f) < 0.001f,
                "Moving pair lost remaining travel after impact");
        require(length(a.velocity + b.velocity) < 0.001f, "Moving pair lost momentum");
        require(!world.sweep_budget_exhausted(), "Elastic pair exhausted swept-event budget");
    }
}
void crossing_and_overtaking() {
    World crossing;
    auto a = agent(0, {-2, 0}, {600, 0}), b = agent(0, {0, -2}, {0, 600});
    a.motion = b.motion = Motion::controlled;
    crossing.add(a); crossing.add(b);
    crossing.step(1.f / 60);
    require(length(crossing.body(0).velocity - a.velocity) > 1 &&
                length(crossing.body(0).position - crossing.body(1).position) > 0.399f,
            "Perpendicular agents crossed without reacting");
    require(!crossing.sweep_budget_exhausted(), "Crossing pair exhausted swept-event budget");
    World overtaking;
    overtaking.add(agent(0, {-2, 0}, {600, 0}));
    overtaking.add(agent(0, {0, 0}, {300, 0}));
    overtaking.step(1.f / 60);
    require(std::abs(overtaking.body(0).velocity.x - 300) < 0.001f &&
                std::abs(overtaking.body(1).velocity.x - 600) < 0.001f &&
                std::abs(overtaking.body(0).position.x - 4.6f) < 0.001f &&
                std::abs(overtaking.body(1).position.x - 8.4f) < 0.001f,
            "Overtaking contact has incorrect timing or remaining travel");
    World together;
    together.add(agent(0, {-2, 0}, {600, 0}));
    together.add(agent(0, {2, 0}, {600, 0}));
    together.step(1.f / 60);
    require(std::abs(together.body(1).position.x - together.body(0).position.x - 4) < 0.001f,
            "Co-moving agents changed separation");
}
void chronological_chain() {
    for (bool reverse : {false, true}) {
        World world;
        std::size_t ids[3];
        for (int k = 0; k < 3; ++k) {
            const int i = reverse ? 2 - k : k;
            ids[i] = world.add(agent(0, {-2.f + static_cast<float>(2 * i), 0}, {i == 0 ? 600.f : 0.f, 0}));
        }
        world.step(1.f / 60);
        require(std::abs(world.body(ids[0]).position.x + 0.4f) < 0.001f &&
                    std::abs(world.body(ids[1]).position.x - 1.6f) < 0.001f &&
                    std::abs(world.body(ids[2]).position.x - 8.8f) < 0.001f,
                "Contact chain was processed out of chronological order");
        require(std::abs(world.body(ids[2]).velocity.x - 600) < 0.001f &&
                    !world.sweep_budget_exhausted(), "Contact chain lost final impulse or exhausted budget");
    }
}
void spinning_tangent_contact() {
    // Captured from the crowded Demo, seed 123, tick 1383. Float contact geometry
    // gives a positive linear approach but a separating contact-point velocity.
    World world;
    Body circle;
    circle.motion = Motion::controlled;
    circle.position = {-13.05731583f, -58.96405029f};
    circle.velocity = {0.1216924191f, 0.1282006353f};
    circle.radius = 0.3199999928f;
    circle.angle = -0.7424513102f;
    circle.angular_velocity = 5;
    circle.friction = 0;
    world.add(circle);
    Body wall;
    wall.shape = Shape::box;
    wall.motion = Motion::fixed;
    wall.position = {-11.66991425f, -59.3385849f};
    wall.half_extents = {0.8739304543f, 0.7712159157f};
    wall.angle = 5.427427769f;
    world.add(wall);
    for (int tick = 0; tick < 120; ++tick) {
        world.step(1.f / 120);
        require(!world.sweep_budget_exhausted(), "Tangent spinning contact exhausted the swept-event budget");
        require(world.candidate_pairs() < 20, "Tangent contact repeatedly rebuilt collision candidates");
    }
    require(length(world.body(0).position - circle.position) > 0.1f,
            "Tangent spinning circle stopped making forward progress");
}

void narrow_resting_gap() {
    // Seed 123, tick 4280: a moving circle touches a sleeping triangular agent
    // and terrain. Nearly opposing normals converged through thousands of
    // zero-time events even though the remaining approach was below solver slop.
    World world;
    auto triangle = regular_polygon(3, 0.32f);
    triangle.motion = Motion::fixed;
    triangle.position = {-18.44842148f, -59.41837311f};
    triangle.angle = -1.797484756f;
    triangle.friction = 0;
    world.add(triangle);
    Body circle;
    circle.position = {-17.84042931f, -59.43058014f};
    circle.velocity = {-0.6518954039f, -1.860282302f};
    circle.radius = 0.32f;
    circle.angle = -1.724964738f;
    circle.angular_velocity = -1.08178556f;
    circle.friction = 0;
    world.add(circle);
    Body terrain;
    terrain.motion = Motion::fixed;
    terrain.position = {-16.73590088f, -59.84957886f};
    terrain.radius = 0.8613312244f;
    terrain.restitution = 0.8f;
    terrain.friction = 0;
    world.add(terrain);
    for (int tick = 0; tick < 120; ++tick) {
        world.step(1.f / 120);
        require(!world.sweep_budget_exhausted(), "Resting gap contacts starved the swept timeline");
        require(world.candidate_pairs() < 64, "Resting gap repeatedly rebuilt collision candidates");
    }
}

void tiny_touching_contact() {
    World world;
    Body wall;
    wall.shape = Shape::box;
    wall.motion = Motion::fixed;
    wall.half_extents = {0.0001f, 1};
    world.add(wall);
    Body circle;
    circle.motion = Motion::controlled;
    circle.radius = 0.0001f;
    circle.position = {-0.00019f, 0};
    circle.velocity = {0.1f, 0};
    world.add(circle);
    world.step(0.01f);
    require(world.body(1).position.x < 0 && std::abs(world.body(1).velocity.x) < 0.000001f,
            "Resting-contact tolerance swallowed a tiny mover's full thickness");
    require(!world.sweep_budget_exhausted(), "Tiny touching contact exhausted swept-event budget");
}

void budget_observability() {
    World world;
    Body wall;
    wall.shape = Shape::box;
    wall.motion = Motion::fixed;
    wall.half_extents = {0.05f, 3};
    wall.restitution = 1; wall.friction = 0;
    wall.position = {-1, 0}; world.add(wall);
    wall.position = {1, 0}; world.add(wall);
    auto ball = agent(0, {}, {60000, 0});
    world.add(ball);
    world.step(1.f / 60);
    require(world.sweep_budget_exhausted() && std::abs(world.body(2).position.x) <= 0.751f,
            "Swept-event fallback failed to report/discard exhausted travel");
    world.body(2).velocity = {};
    world.step(1.f / 60);
    require(!world.sweep_budget_exhausted(), "Swept budget flag was not reset on next step");
    world.body(2).velocity = {60000, 0};
    world.step(1.f / 60);
    world.clear();
    require(!world.sweep_budget_exhausted(), "World clear retained budget exhaustion flag");
}
} // namespace
int main() {
    try {
        pair_query(); fast_head_on(); crossing_and_overtaking(); chronological_chain(); spinning_tangent_contact(); narrow_resting_gap(); tiny_touching_contact(); budget_observability();
        std::cout << "Moving contact contracts passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
