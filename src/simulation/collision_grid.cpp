#include <kinetra/collision_grid.h>
#include <limits>
#include <stdexcept>

namespace kinetra {
namespace {
void validate_grid(const CollisionGrid &grid) {
    if (grid.rows <= 0 || grid.columns <= 0 ||
        grid.cells.size() != std::size_t(grid.rows) * std::size_t(grid.columns) ||
        !std::isfinite(grid.cell_size) || grid.cell_size <= 0 || !finite(grid.origin))
        throw std::invalid_argument("Collision grid requires positive dimensions and matching occupancy");
}
} // namespace
void collide_grid(Body &body, const CollisionGrid &grid) {
    validate_grid(grid);
    validate(body);
    // Query only overlapping cells, then let the engine calculate contacts and response.
    for (int pass = 0; pass < 8; ++pass) {
        const auto area = bounds(body);
        const int first_row = std::max(0, int(std::floor((area.min.y - grid.origin.y) / grid.cell_size)));
        const int last_row =
            std::min(grid.rows - 1, int(std::floor((area.max.y - grid.origin.y) / grid.cell_size)));
        const int first_col = std::max(0, int(std::floor((area.min.x - grid.origin.x) / grid.cell_size)));
        const int last_col =
            std::min(grid.columns - 1, int(std::floor((area.max.x - grid.origin.x) / grid.cell_size)));
        for (int row = first_row; row <= last_row; ++row)
            for (int col = first_col; col <= last_col; ++col) {
                if (!grid.cells[row * grid.columns + col])
                    continue;
                Body wall;
                wall.shape = Shape::box;
                wall.motion = Motion::fixed;
                wall.position = grid.origin + Vec2{(static_cast<float>(col) + 0.5f) * grid.cell_size,
                                                   (static_cast<float>(row) + 0.5f) * grid.cell_size};
                wall.half_extents = {grid.cell_size / 2, grid.cell_size / 2};
                wall.friction = 0;
                if (const auto contact = collide(body, wall))
                    resolve_contact(body, wall, *contact);
            }
    }
}

std::optional<float> segment_grid(Vec2 start, Vec2 end, const CollisionGrid &grid) {
    validate_grid(grid);
    if (!finite(start) || !finite(end))
        throw std::invalid_argument("Grid segment endpoints must be finite");
    const auto delta = end - start;
    int column = int(std::floor((start.x - grid.origin.x) / grid.cell_size));
    int row = int(std::floor((start.y - grid.origin.y) / grid.cell_size));
    const auto blocked = [&](int y, int x) {
        return x < 0 || y < 0 || x >= grid.columns || y >= grid.rows || grid.cells[y * grid.columns + x] != 0;
    };
    if (blocked(row, column)) {
        return 0;
    }
    const int step_x = delta.x > 0 ? 1 : -1, step_y = delta.y > 0 ? 1 : -1;
    const float infinity = std::numeric_limits<float>::infinity();
    const float increment_x = delta.x == 0 ? infinity : grid.cell_size / std::abs(delta.x);
    const float increment_y = delta.y == 0 ? infinity : grid.cell_size / std::abs(delta.y);
    float next_x =
        delta.x == 0
            ? infinity
            : (grid.origin.x + static_cast<float>(column + (step_x > 0)) * grid.cell_size - start.x) /
                  delta.x;
    float next_y =
        delta.y == 0
            ? infinity
            : (grid.origin.y + static_cast<float>(row + (step_y > 0)) * grid.cell_size - start.y) / delta.y;
    for (int crossed = 0; crossed < grid.rows + grid.columns + 4; ++crossed) {
        const float t = std::min(next_x, next_y);
        if (t > 1)
            return std::nullopt;
        const bool cross_x = next_x <= next_y, cross_y = next_y <= next_x;
        if ((cross_x && blocked(row, column + step_x)) || (cross_y && blocked(row + step_y, column)) ||
            (cross_x && cross_y && blocked(row + step_y, column + step_x))) {
            return t;
        }
        if (cross_x) {
            column += step_x;
            next_x += increment_x;
        }
        if (cross_y) {
            row += step_y;
            next_y += increment_y;
        }
    }
    return 0;
}
} // namespace kinetra
