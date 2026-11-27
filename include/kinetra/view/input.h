#pragma once

#include <kinetra/simulation/math.h>

#include <array>
#include <cstddef>
#include <string>

// Plain input and color types shared by the runtime and headless application logic. They
// need no window: a test or a logic layer fills an Input by hand.
namespace kinetra {

/// Keyboard keys the runtime reports; other keys are ignored. Both Shift and both Ctrl keys
/// map to `shift` and `control`. `count` is the number of keys, not a key.
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
    backspace,
    count
};
/// Input state of one frame, filled by Runtime::poll().
///
/// "Pressed" and "clicked" flags and the wheel describe events since the previous poll;
/// "held" and "down" flags describe the current state. Losing window focus releases every
/// held key and button. Mouse positions are logical window pixels, Y down.
struct Input {
    /// `held`: keys currently down. `pressed`: keys that went down this frame (key repeat
    /// excluded). Indexed by Key; prefer down() and hit().
    std::array<bool, static_cast<std::size_t>(Key::count)> held{}, pressed{};
    /// Pointer position in logical window pixels.
    Vec2 mouse{};
    /// Left button: pressed this frame, and currently held.
    bool clicked{}, mouse_down{};
    /// Right button: pressed this frame, and currently held.
    bool right_clicked{}, right_mouse_down{};
    /// Wheel notches this frame; positive scrolls away from the user. Can be fractional.
    float wheel{};
    /// Text typed since the previous poll, in order, as UTF-8: what the keys produce with the
    /// keyboard layout and Shift applied, keypad digits included. Keys that produce no text,
    /// such as Enter and Backspace, are reported only through `pressed`.
    std::string text;
    /// The window lost focus since the previous poll. The keys and buttons that are no longer
    /// held were released by that, not by the user.
    bool focus_lost{};
    /// Whether `key` is held. Throws std::out_of_range for Key::count.
    bool down(Key key) const { return held.at(static_cast<std::size_t>(key)); }
    /// Whether `key` went down this frame. Throws std::out_of_range for Key::count.
    bool hit(Key key) const { return pressed.at(static_cast<std::size_t>(key)); }
};
/// RGBA color, each channel in [0, 1]; alpha 1 is opaque.
struct Color {
    float r{}, g{}, b{}, a{1};
};

} // namespace kinetra
