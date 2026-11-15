#pragma once

#include "camera.h"
#include <array>
#include <memory>
#include <string>

namespace kinetra {

enum class Key {
    w,
    a,
    s,
    d,
    q,
    e,
    up,
    down,
    left,
    right,
    space,
    r,
    g,
    tab,
    f,
    p,
    t,
    c,
    escape,
    h,
    enter,
    m,
    shift,
    v,
    backquote,
    one,
    two,
    three,
    four,
    five,
    six,
    control,
    count
};
struct Input {
    std::array<bool, static_cast<std::size_t>(Key::count)> held{}, pressed{};
    Vec2 mouse{};
    bool clicked{}, mouse_down{};
    float wheel{};
    bool down(Key key) const { return held.at(static_cast<std::size_t>(key)); }
    bool hit(Key key) const { return pressed.at(static_cast<std::size_t>(key)); }
};
struct Color {
    float r{}, g{}, b{}, a{1};
};

/// SDL2/OpenGL 3.3 window, input and batched primitive drawing. Own on the main
/// thread; only one Runtime may exist at a time. Throws runtime_error on setup,
/// shader or rendering initialization failures. No simulation dependency.
class Runtime {
  public:
    explicit Runtime(const std::string &title = "Kinetra", int width = 1280, int height = 800);
    ~Runtime();
    Runtime(const Runtime &) = delete;
    Runtime &operator=(const Runtime &) = delete;
    /// Disable Escape shutdown when the application supplies its own exit action.
    bool poll(bool close_on_escape = true);
    const Input &input() const;
    void title(const std::string &text);
    void begin(Color background = {0.035f, 0.045f, 0.07f, 1});
    void circle(Vec2 center, float radius, Color color);
    void box(Vec2 center, Vec2 half_extents, float angle, Color color);
    void line(Vec2 from, Vec2 to, float width, Color color);
    void end();
    Camera2D camera;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color color);
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc);
};

} // namespace kinetra
