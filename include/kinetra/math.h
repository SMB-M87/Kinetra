#pragma once

#include <algorithm>
#include <cmath>

namespace kinetra {

struct Vec2 {
    float x{}, y{};
    constexpr Vec2 operator+(Vec2 v) const { return {x + v.x, y + v.y}; }
    constexpr Vec2 operator-(Vec2 v) const { return {x - v.x, y - v.y}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2 &operator+=(Vec2 v) {
        x += v.x;
        y += v.y;
        return *this;
    }
    Vec2 &operator-=(Vec2 v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }
};

constexpr float dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}
constexpr float cross(Vec2 a, Vec2 b) {
    return a.x * b.y - a.y * b.x;
}
constexpr Vec2 perpendicular(Vec2 a) {
    return {-a.y, a.x};
}
inline float length(Vec2 v) {
    return std::sqrt(dot(v, v));
}
inline Vec2 normalized(Vec2 v) {
    const float n = length(v);
    return n > 0 ? v / n : Vec2{};
}
inline Vec2 limited(Vec2 v, float maximum) {
    const float n = length(v);
    return n > maximum && n > 0 ? v * (maximum / n) : v;
}
inline Vec2 rotated(Vec2 v, float angle) {
    const float c = std::cos(angle), s = std::sin(angle);
    return {c * v.x - s * v.y, s * v.x + c * v.y};
}
inline bool finite(Vec2 v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

/// Euclidean distance; zero-length segments reduce to a point.
inline float distance(Vec2 a, Vec2 b) {
    return length(a - b);
}
inline float distance_to_segment(Vec2 p, Vec2 a, Vec2 b) {
    const auto d = b - a;
    const float squared = dot(d, d);
    return length(p - (a + d * (squared > 0 ? std::clamp(dot(p - a, d) / squared, 0.f, 1.f) : 0.f)));
}
inline float signed_angle(Vec2 a, Vec2 b) {
    return std::atan2(cross(a, b), dot(a, b));
}
inline Vec2 lerp(Vec2 a, Vec2 b, float t) {
    return a + (b - a) * t;
}
struct Segment {
    Vec2 start, end;
};
struct Circle {
    Vec2 center;
    float radius;
};

struct Aabb {
    Vec2 min, max;
    bool overlaps(Aabb b) const {
        return min.x <= b.max.x && max.x >= b.min.x && min.y <= b.max.y && max.y >= b.min.y;
    }
};

} // namespace kinetra
