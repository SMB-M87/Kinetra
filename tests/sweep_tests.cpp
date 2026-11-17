#include <kinetra/physics.h>
#include <iostream>
#include <stdexcept>

using namespace kinetra;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
Body wall() {
    Body body;
    body.shape = Shape::box;
    body.motion = Motion::fixed;
    body.half_extents = {0.05f, 2};
    body.restitution = 1;
    body.friction = 0;
    return body;
}
void query_contracts() {
    Body circle;
    circle.radius = 0.1f;
    circle.position = {-2, 0};
    const auto obstacle = wall();
    const auto hit = sweep_circle(circle, {2, 0}, obstacle);
    require(hit && std::abs(hit->fraction - 0.4625f) < 0.00001f &&
                length(hit->contact.normal - Vec2{1, 0}) < 0.00001f &&
                std::abs(hit->contact.point.x + 0.05f) < 0.00001f,
            "Thin-wall sweep has incorrect time/normal/point");
    const float angle = 0.63f;
    const Vec2 offset{3, -4};
    auto rotated_circle = circle, rotated_wall = obstacle;
    rotated_circle.position = rotated(circle.position, angle) + offset;
    rotated_wall.position = offset;
    rotated_wall.angle = angle;
    const auto transformed = sweep_circle(rotated_circle, rotated(Vec2{2, 0}, angle) + offset, rotated_wall);
    require(transformed && std::abs(transformed->fraction - hit->fraction) < 0.00001f &&
                length(transformed->contact.normal - rotated(hit->contact.normal, angle)) < 0.00001f &&
                length(transformed->contact.point - (rotated(hit->contact.point, angle) + offset)) < 0.00001f,
            "Sweep is not rigid-transform invariant");
    require(!sweep_circle(circle, {-3, 0}, obstacle), "Separating sweep hit a wall");
    require(!sweep_circle(circle, circle.position, obstacle), "Stationary separated sweep hit a wall");
    circle.position = {};
    const auto inside = sweep_circle(circle, {2, 0}, obstacle);
    require(inside && inside->fraction == 0 && inside->contact.penetration > 0,
            "Initial overlap was not reported");
    circle.position = {-2, 2.1f};
    require(sweep_circle(circle, {2, 2.1f}, obstacle).has_value(), "Grazing corner sweep missed");
    circle.position.y = 2.11f;
    require(!sweep_circle(circle, {2, 2.11f}, obstacle), "Near-corner miss became a false hit");
    Body target;
    target.radius = 0.5f;
    circle.position = {-2, 0};
    const auto round = sweep_circle(circle, {2, 0}, target);
    require(round && std::abs(round->fraction - 0.35f) < 0.00001f, "Circle sweep time is incorrect");
    target = regular_polygon(3, 1);
    circle.position = {-2, 0};
    const auto polygon = sweep_circle(circle, {2, 0}, target);
    require(polygon && std::abs(polygon->fraction - 0.35f) < 0.00001f,
            "Triangle face sweep used an incorrect envelope");
    circle.position = {2, 0};
    const auto vertex = sweep_circle(circle, {-2, 0}, target);
    require(vertex && std::abs(vertex->fraction - 0.225f) < 0.00001f,
            "Triangle vertex sweep missed rounded corner");
    bool rejected = false;
    try { sweep_circle(target, {}, obstacle); }
    catch (const std::invalid_argument &) { rejected = true; }
    require(rejected, "Noncircle sweep mover accepted");
}
void world_impacts() {
    for (const auto motion : {Motion::dynamic, Motion::controlled})
        for (const float dt : {1.f / 60, 1.f / 120}) {
            World world;
            world.add(wall());
            Body ball;
            ball.motion = motion;
            ball.radius = 0.1f;
            ball.position = {-2, 0};
            ball.velocity = {600, 0};
            ball.restitution = 1;
            ball.friction = 0;
            world.add(ball);
            world.step(dt);
            require(world.body(1).position.x <= -0.1499f, "Fast circle tunneled through a fixed wall");
            require(world.body(0).position.x == 0, "Swept impact moved fixed obstacle");
            if (motion == Motion::dynamic) {
                require(std::abs(world.body(1).velocity.x + 600) < 0.001f, "Elastic swept bounce lost speed");
                require(std::abs(world.body(1).position.x - (-2 + 3.7f - 600 * dt)) < 0.001f,
                        "Swept bounce did not consume remaining travel");
            } else {
                require(length(world.body(1).velocity) < 0.001f, "Controlled circle bounced");
            }
        }
    for (int shape = 0; shape < 3; ++shape) {
        World world;
        Body obstacle = shape == 2 ? regular_polygon(3, 1) : wall();
        if (shape == 1) {
            obstacle.shape = Shape::circle;
            obstacle.radius = 0.5f;
        }
        obstacle.motion = Motion::fixed;
        obstacle.angle = 0.63f;
        world.add(obstacle);
        Body ball;
        ball.radius = 0.1f;
        ball.motion = Motion::controlled;
        ball.position = rotated({-2, 0}, obstacle.angle);
        ball.velocity = rotated({600, 0}, obstacle.angle);
        world.add(ball);
        world.step(1.f / 60);
        const auto local = rotated(world.body(1).position, -obstacle.angle);
        const float limit = shape == 0 ? -0.15f : -0.6f;
        require(local.x <= limit + 0.0001f && length(world.body(1).velocity) < 0.001f,
                "World sweep failed against rotated fixed geometry");
    }
    World corridor;
    auto left = wall(), right = wall();
    left.position = {-1, 0}; right.position = {1, 0};
    corridor.add(left); corridor.add(right);
    Body ball;
    ball.radius = 0.1f;
    ball.velocity = {600, 0};
    ball.restitution = 1; ball.friction = 0;
    corridor.add(ball);
    corridor.step(1.f / 60);
    require(std::abs(corridor.body(2).position.x) <= 0.851f &&
                std::abs(std::abs(corridor.body(2).velocity.x) - 600) < 0.001f,
            "Multiple swept impacts escaped corridor or lost speed");
    // Very many bounces exhaust the bounded impact budget: keep the body inside.
    corridor.body(2).velocity = {60000, 0};
    corridor.step(1.f / 60);
    require(std::abs(corridor.body(2).position.x) <= 0.851f, "Impact budget exhaustion allowed tunneling");
}
} // namespace
int main() {
    try {
        query_contracts();
        world_impacts();
        std::cout << "Circle sweep contracts passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
