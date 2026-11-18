#include <iostream>
#include <kinetra/physics.h>
#include <limits>
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
    body.half_extents = {0.05f, 3};
    body.motion = Motion::fixed;
    body.restitution = 1;
    body.friction = 0;
    return body;
}
Body mover(int variant) {
    Body body;
    if (variant >= 2)
        body = regular_polygon(variant == 2 ? 3 : 6, 0.2f);
    else
        body.shape = variant == 0 ? Shape::circle : Shape::box;
    body.radius = 0.2f;
    body.half_extents = {0.2f, 0.3f};
    body.position = {-2, 0};
    body.restitution = 1;
    body.friction = 0;
    return body;
}
void query_contracts() {
    for (int variant = 0; variant < 4; ++variant) {
        const auto body = mover(variant);
        const auto obstacle = wall();
        const auto hit = sweep_translation(body, {2, 0}, obstacle);
        require(hit && std::abs(hit->fraction - 0.4375f) < 0.00001f &&
                    length(hit->contact.normal - Vec2{1, 0}) < 0.0001f &&
                    std::abs(hit->contact.point.x + 0.05f) < 0.0001f,
                "Convex thin-wall sweep has incorrect time/normal/point");
        require(!sweep_translation(body, {-3, 0}, obstacle), "Separating convex sweep hit wall");
        require(!sweep_translation(body, body.position, obstacle),
                "Separated stationary convex sweep hit wall");
        auto touching = body;
        touching.position.x = -0.2f;
        const auto initial = sweep_translation(touching, {2, 0}, obstacle);
        require(initial && initial->fraction == 0, "Convex sweep lost initial overlap");
        const float angle = 0.63f;
        const Vec2 offset{3, -4};
        auto rotated_body = body, rotated_wall = obstacle;
        rotated_body.position = rotated(body.position, angle) + offset;
        rotated_body.angle = rotated_wall.angle = angle;
        rotated_wall.position = offset;
        const auto transformed =
            sweep_translation(rotated_body, rotated(Vec2{2, 0}, angle) + offset, rotated_wall);
        require(transformed && std::abs(transformed->fraction - hit->fraction) < 0.00001f &&
                    length(transformed->contact.normal - rotated(hit->contact.normal, angle)) < 0.0001f &&
                    length(transformed->contact.point - (rotated(hit->contact.point, angle) + offset)) <
                        0.0001f,
                "Convex sweep is not rigid-transform invariant");
        for (const bool round : {false, true}) {
            auto target = round ? Body{} : regular_polygon(3, 1);
            target.radius = 0.5f;
            const auto result = sweep_translation(body, {2, 0}, target);
            require(result && std::abs(result->fraction - 0.325f) < 0.0001f,
                    "Convex sweep failed against circle/polygon");
        }
    }
    auto box = mover(1);
    box.angle = 0.4f;
    const auto hit = sweep_translation(box, {2, 0}, wall());
    const float extent = std::cos(box.angle) * 0.2f + std::sin(box.angle) * 0.3f;
    require(hit && std::abs(hit->fraction - (2 - 0.05f - extent) / 4) < 0.00001f,
            "Sweep ignored mover orientation");
    auto small = wall();
    small.half_extents = {0.2f, 0.2f};
    box.angle = 0;
    box.position = {-2, 0.6f};
    require(!sweep_translation(box, {2, 0.6f}, small), "Convex near miss became a false hit");
    box.position = {-2, 0};
    small.position = {0, 1.5f};
    box.position = {-2, -2};
    require(!sweep_translation(box, {2, 2}, small), "Disjoint sweep intervals became contact");
    bool rejected = false;
    try {
        sweep_translation(box, {std::numeric_limits<float>::infinity(), 0}, small);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    require(rejected, "Translation sweep accepted nonfinite endpoint");
}
void world_contracts() {
    for (int variant = 0; variant < 4; ++variant)
        for (auto motion : {Motion::controlled, Motion::dynamic}) {
            World world;
            world.add(wall());
            auto body = mover(variant);
            body.motion = motion;
            body.velocity = {600, 0};
            world.add(body);
            world.step(1.f / 60);
            require(world.body(1).position.x <= -0.2499f, "Convex mover tunneled through thin wall");
            require(world.body(1).velocity.x <= 0, "Convex swept impact did not remove approach velocity");
            if (motion == Motion::controlled)
                require(length(world.body(1).velocity) < 0.001f, "Controlled convex body bounced");
        }
    World world;
    auto left = wall(), right = wall();
    left.position = {-1, 0};
    right.position = {1, 0};
    world.add(left);
    world.add(right);
    auto body = mover(1);
    body.position = {};
    body.velocity = {600, 0};
    world.add(body);
    world.step(1.f / 60);
    require(std::abs(world.body(2).position.x) <= 0.751f &&
                std::abs(std::abs(world.body(2).velocity.x) - 600) < 0.001f,
            "Convex repeated impacts escaped corridor or lost speed");
    // A rotating body with no obstacles keeps the existing free-integration contract.
    World free;
    body = mover(1);
    body.velocity = {6, 2};
    body.angular_velocity = 4;
    free.add(body);
    free.step(1.f / 60);
    require(length(free.body(0).position - (body.position + body.velocity / 60)) < 0.0001f &&
                std::abs(free.body(0).angle - 4.f / 60) < 0.0001f,
            "Unsupported rotational sweep changed free integration");
}
} // namespace
int main() {
    try {
        query_contracts();
        world_contracts();
        std::cout << "Translation sweep contracts passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
