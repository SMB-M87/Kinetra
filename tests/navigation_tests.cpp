#include <iostream>
#include <kinetra/navigation.h>
#include <limits>
#include <stdexcept>

using namespace kinetra;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F operation) {
    try {
        operation();
    } catch (const std::invalid_argument &) {
        return;
    }
    throw std::runtime_error("Invalid navigation settings accepted");
}
} // namespace
int main() {
    try {
        rejects([] { Navigation invalid(0, 1); });
        rejects([] { Navigation invalid(1, -1); });
        rejects([] { Navigation invalid(std::numeric_limits<float>::infinity(), 1); });
        Navigation navigation(0.5f, 0.2f);
        Body obstacle;
        obstacle.shape = Shape::box;
        obstacle.position = {0, 0};
        obstacle.half_extents = {0.5f, 2};
        navigation.rebuild({{-5, -5}, {5, 5}}, {obstacle});
        require(!navigation.clear({-2, 0}, {2, 0}), "Sweep crossed obstacle");
        auto path = navigation.path({-2, 0}, {2.123f, 0.123f});
        require(!path.empty() && distance(path.back(), {2.123f, 0.123f}) < 0.00001f,
                "Free goal lost precision");
        Vec2 previous{-2, 0};
        for (auto point : path) {
            require(navigation.clear(previous, point), "Path contains obstructed segment");
            previous = point;
        }
        auto projected = navigation.path({-2, 0}, {});
        require(!projected.empty() && navigation.clear(projected.back(), projected.back()),
                "Occupied endpoint not projected");
        require(navigation.path({-2, 0}, {std::numeric_limits<float>::quiet_NaN(), 0}).empty(),
                "NaN goal accepted");
        require(distance_to_segment({1, 0}, {}, {}) == 1, "Degenerate segment distance");
        require(length(normalized({})) == 0, "Zero normalization");
        require(!finite({std::numeric_limits<float>::infinity(), 0}), "Infinite vector accepted as finite");
        require(distance(lerp({}, {2, 4}, 0.5f), {1, 2}) == 0, "Interpolation");
        require(Aabb{{0, 0}, {1, 1}}.overlaps({{1, 1}, {2, 2}}), "Touching AABBs missed");
        std::cout << "PASS public navigation and math boundaries\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
