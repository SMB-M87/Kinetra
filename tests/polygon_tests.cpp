#include <kinetra/navigation.h>
#include <kinetra/steering.h>
#include <iostream>
#include <stdexcept>

using namespace kinetra;
void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
void invalid(Body body) {
    try { validate(body); }
    catch (const std::invalid_argument &) { return; }
    throw std::runtime_error("Invalid polygon accepted");
}
int main() {
    try {
        for (int sides : {3, 5, 6, 32}) {
            auto polygon = regular_polygon(sides, 1);
            polygon.angle = 0.37f;
            polygon.position = {2, -3};
            const auto box = bounds(polygon);
            for (auto vertex : polygon.vertices) {
                const auto p = polygon.position + rotated(vertex, polygon.angle);
                require(p.x >= box.min.x && p.x <= box.max.x && p.y >= box.min.y && p.y <= box.max.y,
                        "Polygon bounds exclude a vertex");
            }
            Body circle;
            circle.position = polygon.position;
            circle.radius = 0.1f;
            auto contact = collide(circle, polygon);
            require(contact && contact->penetration > 0 && std::abs(length(contact->normal) - 1) < 0.0001f,
                    "Contained circle lost its exit contact");
            resolve_contact(circle, polygon, *contact);
            require(finite(circle.position), "Containment correction is not finite");
        }
        auto triangle = regular_polygon(3, 1);
        Body circle;
        circle.radius = 0.1f;
        circle.position = {-0.8f, 0};
        require(!collide(triangle, circle), "Triangle incorrectly uses its bounding circle as collider");
        circle.position = {1.05f, 0};
        auto contact = collide(triangle, circle), reversed = collide(circle, triangle);
        require(contact && reversed && length(contact->normal + reversed->normal) < 0.0001f,
                "Vertex contact is missing or asymmetric");
        auto hexagon = regular_polygon(6, 1);
        hexagon.position = {1.4f, 0.15f};
        hexagon.angle = 0.2f;
        Body box_body;
        box_body.shape = Shape::box;
        for (auto other : {hexagon, box_body}) {
            const auto hit = collide(triangle, other);
            require(hit && hit->penetration > 0, "Polygon pair contact missing");
            const float angle = 0.73f;
            auto a = triangle, b = other;
            a.angle += angle; b.angle += angle;
            a.position = rotated(a.position, angle) + Vec2{4, -2};
            b.position = rotated(b.position, angle) + Vec2{4, -2};
            const auto transformed = collide(a, b);
            require(transformed && std::abs(transformed->penetration - hit->penetration) < 0.0001f &&
                        length(transformed->normal - rotated(hit->normal, angle)) < 0.0001f &&
                        length(transformed->point - (rotated(hit->point, angle) + Vec2{4, -2})) < 0.0001f,
                    "Polygon contact is not rigid-transform invariant");
        }
        auto square = regular_polygon(4, 1);
        apply_body_impulse(square, {0, 1}, {1, 0});
        require(std::abs(square.angular_velocity - 3) < 0.0001f, "Polygon inertia is incorrect");
        square = regular_polygon(4, 1);
        square.vertices.pop_back(); invalid(square);
        square = regular_polygon(4, 1);
        std::reverse(square.vertices.begin(), square.vertices.end()); invalid(square);
        square = regular_polygon(4, 1);
        square.vertices[1] = square.vertices[0]; invalid(square);
        square = regular_polygon(5, 1);
        square.vertices[1] = {}; invalid(square);
        square = regular_polygon(4, 1);
        for (auto &v : square.vertices) v.x += 1;
        invalid(square);
        Navigation navigation(0.25f, 0.1f);
        triangle.motion = Motion::fixed;
        std::vector<Body> obstacles{triangle};
        navigation.rebuild({{-4, -4}, {4, 4}}, obstacles);
        require(!navigation.clear({-2, 0}, {2, 0}), "Navigation crossed polygon obstacle");
        require(navigation.clear({-2, 2}, {2, 2}), "Navigation blocked a clear polygon bypass");
        require(finite(steer(regular_polygon(3, 0.3f), {3, 0}, obstacles)), "Polygon steering is not finite");
        World world;
        Body floor;
        floor.shape = Shape::box; floor.motion = Motion::fixed;
        floor.position = {0, -1}; floor.half_extents = {5, 0.5f};
        world.add(floor);
        auto falling = regular_polygon(6, 0.5f);
        falling.position = {0, 2}; falling.restitution = 0;
        world.add(falling); world.gravity = {0, -9.81f};
        for (int i = 0; i < 600; ++i) world.step(1.f / 120);
        require(finite(world.body(1).position) && bounds(world.body(1)).min.y > -0.52f &&
                    length(world.body(1).velocity) < 0.1f, "Polygon floor contact failed to settle");
        std::cout << "Polygon contracts passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
