#pragma once

#include <algorithm>
#include <cmath>

namespace kinetra {

/// Two-component float vector for positions, directions, velocities and extents.
///
/// A plain value type: the arithmetic follows IEEE floating-point rules and never validates,
/// so infinities and NaN propagate. Boundaries that need finite input check finite().
/// Units are whatever the caller uses; the simulation uses world units and seconds.
struct Vec2 {
    /// Horizontal and vertical component. In world space Y points up.
    float x{}, y{};
    /// Component-wise sum.
    constexpr Vec2 operator+(Vec2 v) const { return {x + v.x, y + v.y}; }
    /// Component-wise difference.
    constexpr Vec2 operator-(Vec2 v) const { return {x - v.x, y - v.y}; }
    /// Negation of both components.
    constexpr Vec2 operator-() const { return {-x, -y}; }
    /// Both components scaled by `s`.
    constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
    /// Both components divided by `s`; division by zero yields infinities or NaN.
    constexpr Vec2 operator/(float s) const { return {x / s, y / s}; }
    /// Adds `v` in place.
    Vec2 &operator+=(Vec2 v) {
        x += v.x;
        y += v.y;
        return *this;
    }
    /// Subtracts `v` in place.
    Vec2 &operator-=(Vec2 v) {
        x -= v.x;
        y -= v.y;
        return *this;
    }
};

/// Dot product.
constexpr float dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}
/// Z component of the 3D cross product; positive when `b` lies counterclockwise of `a`.
constexpr float cross(Vec2 a, Vec2 b) {
    return a.x * b.y - a.y * b.x;
}
/// `a` rotated a quarter turn counterclockwise.
constexpr Vec2 perpendicular(Vec2 a) {
    return {-a.y, a.x};
}
/// Euclidean length.
inline float length(Vec2 v) {
    return std::sqrt(dot(v, v));
}
/// Unit vector in the direction of `v`; the zero vector for a zero-length input.
inline Vec2 normalized(Vec2 v) {
    const float n = length(v);
    return n > 0 ? v / n : Vec2{};
}
/// `v` scaled down to length `maximum` when it is longer; otherwise unchanged.
inline Vec2 limited(Vec2 v, float maximum) {
    const float n = length(v);
    return n > maximum && n > 0 ? v * (maximum / n) : v;
}
/// `v` rotated by `angle` radians; positive angles rotate counterclockwise.
inline Vec2 rotated(Vec2 v, float angle) {
    const float c = std::cos(angle), s = std::sin(angle);
    return {c * v.x - s * v.y, s * v.x + c * v.y};
}
/// Whether both components are finite (neither infinite nor NaN).
inline bool finite(Vec2 v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

/// Angle in radians of the point (`x`, `y`), in [-pi, pi], with std::atan2's quadrants, signed
/// zeros, infinities and NaN. Use it instead of std::atan2 on floats in simulation code: the
/// result is the correctly rounded float on every C library, except in rare double-rounding
/// cases near a midpoint between two floats. `atan2f` itself differs in the last bit between,
/// for example, glibc 2.39 and 2.41, and one bit changes a chaotic trajectory. Precondition: IEEE-conforming float
/// semantics; `-ffast-math` or `-funsafe-math-optimizations` may let the compiler narrow the
/// double call back to the C library's float function. The function is inline, so the flags
/// are the caller's.
inline float atan2(float y, float x) {
    // A double atan2 is accurate to about one double ulp on every C library, 29 bits beyond a
    // float, so one rounding to float gives the correctly rounded value except when the exact
    // angle lies within that ulp of the midpoint between two floats.
    return static_cast<float>(std::atan2(static_cast<double>(y), static_cast<double>(x)));
}
/// Arc sine of `x` in radians, in [-pi/2, pi/2]; NaN outside [-1, 1]. Correctly rounded on
/// every C library, for the same reason, in the same way and with the same exception and
/// precondition as atan2(); GCC and Clang do narrow this one to `asinf` under `-ffast-math`.
inline float asin(float x) {
    return static_cast<float>(std::asin(static_cast<double>(x)));
}

/// Euclidean distance between two points.
inline float distance(Vec2 a, Vec2 b) {
    return length(a - b);
}
/// Shortest distance from `p` to the segment from `a` to `b`; a zero-length segment is a point.
inline float distance_to_segment(Vec2 p, Vec2 a, Vec2 b) {
    const auto d = b - a;
    const float squared = dot(d, d);
    return length(p - (a + d * (squared > 0 ? std::clamp(dot(p - a, d) / squared, 0.f, 1.f) : 0.f)));
}
/// Angle in radians from `a` to `b`, in [-pi, pi]; positive is counterclockwise.
inline float signed_angle(Vec2 a, Vec2 b) {
    return kinetra::atan2(cross(a, b), dot(a, b));
}
/// Linear interpolation: `a` at `t` = 0, `b` at `t` = 1. `t` is not clamped.
inline Vec2 lerp(Vec2 a, Vec2 b, float t) {
    return a + (b - a) * t;
}
/// Angle in radians between `a` (`t` = 0) and `b` (`t` = 1) along the shorter way around,
/// so 3.1 to -3.1 passes through pi rather than zero. The result is not wrapped to [-pi, pi].
inline float lerp_angle(float a, float b, float t) {
    constexpr float pi = 3.14159265358979f;
    const float difference = std::remainder(b - a, 2 * pi);
    return a + difference * t;
}
/// Line segment from `start` to `end`.
struct Segment {
    Vec2 start, end;
};
/// Circle by center and radius, in the caller's units.
struct Circle {
    Vec2 center;
    float radius;
};

/// Axis-aligned bounding box. Valid when `min` <= `max` on both axes; not checked here.
struct Aabb {
    Vec2 min, max;
    /// Whether the boxes intersect. Touching edges count as overlap.
    bool overlaps(Aabb b) const {
        return min.x <= b.max.x && max.x >= b.min.x && min.y <= b.max.y && max.y >= b.min.y;
    }
};

} // namespace kinetra
