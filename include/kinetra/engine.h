#pragma once

#include "camera.h"
#include "timing.h"
#include <array>
#include <memory>
#include <span>
#include <string>

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
    /// Whether `key` is held. Throws std::out_of_range for Key::count.
    bool down(Key key) const { return held.at(static_cast<std::size_t>(key)); }
    /// Whether `key` went down this frame. Throws std::out_of_range for Key::count.
    bool hit(Key key) const { return pressed.at(static_cast<std::size_t>(key)); }
};
/// RGBA color, each channel in [0, 1]; alpha 1 is opaque.
struct Color {
    float r{}, g{}, b{}, a{1};
};

/// What spaces the frames Runtime::end() presents.
enum class FramePacing {
    /// The platform accepted a swap interval: presenting waits for the display.
    vsync,
    /// No swap interval is available; end() waits until one refresh interval has passed
    /// since the previous frame.
    limited,
    /// No swap interval and no waiting: frames are presented as fast as they are drawn.
    uncapped,
};

/// SDL2/OpenGL 3.3 window, input and batched primitive drawing. Own on the main
/// thread; only one Runtime may exist at a time. Throws runtime_error on setup,
/// shader or rendering initialization failures. No simulation dependency.
///
/// A frame is poll(), then begin(), any number of drawing calls, and end(). Drawing calls
/// take world coordinates and go through `camera`; they only fill a batch, which end()
/// uploads, draws and presents.
class Runtime {
  public:
    /// Opens a resizable window of `width` x `height` logical pixels with the Kinetra icon.
    /// The icon is not shown where the platform ignores window icons: native Wayland, and
    /// the title bar that WSLg draws (its taskbar entry does show it). Throws
    /// std::invalid_argument for nonpositive dimensions and std::runtime_error when another
    /// Runtime is active or SDL, the OpenGL 3.3 context or the shaders fail.
    explicit Runtime(const std::string &title = "Kinetra", int width = 1280, int height = 800);
    /// Destroys the window and the OpenGL context, and allows a new Runtime.
    ~Runtime();
    Runtime(const Runtime &) = delete;
    Runtime &operator=(const Runtime &) = delete;
    /// Processes pending window events into input() and sets `camera.viewport` to the window
    /// size. Returns false once the window was closed, or this frame when Escape was pressed
    /// and `close_on_escape` is true.
    /// Disable Escape shutdown when the application supplies its own exit action.
    bool poll(bool close_on_escape = true);
    /// Input of the last poll(); the reference stays valid for the Runtime's lifetime.
    const Input &input() const;
    /// Sets the window title; repeating the current title costs nothing.
    void title(const std::string &text);
    /// Starts a frame: clears to `background` and empties the batch. Throws
    /// std::invalid_argument for an invalid camera.
    void begin(Color background = {0.035f, 0.045f, 0.07f, 1});
    /// Filled antialiased circle; `radius` in world units.
    void circle(Vec2 center, float radius, Color color);
    /// Filled rectangle of `half_extents` world units, rotated by `angle` radians.
    void box(Vec2 center, Vec2 half_extents, float angle, Color color);
    /// Filled convex polygon in world coordinates; caller supplies convex ordered vertices.
    /// Throws std::invalid_argument for fewer than three or nonfinite vertices.
    void polygon(std::span<const Vec2> vertices, Color color);
    /// Straight line of `width` world units.
    void line(Vec2 from, Vec2 to, float width, Color color);
    /// Uploads and draws the batch, then presents the frame. Without VSync it then waits for
    /// the frame limiter, unless that was switched off (see pacing()). From the second frame
    /// on, also records the time since the previous end() in frame_times().
    void end();
    /// What spaces the frames: `vsync` when the platform accepted a swap interval when the
    /// window was opened; otherwise `limited`, or `uncapped` after limit_frames(false).
    FramePacing pacing() const;
    /// Chooses between `limited` (true, the default) and `uncapped` on a platform without
    /// VSync; does nothing where presenting already waits for the display. An application
    /// that renders frames for a file rather than for a viewer switches the limit off.
    void limit_frames(bool limited);
    /// Seconds between frames that the display asks for: one over the refresh rate of the
    /// window's display when it was opened, 1/60 when that is unknown. The limiter aims for
    /// this interval.
    double frame_interval() const;
    /// Wall-clock time between the last presented frames, oldest first. With VSync or the
    /// frame limiter this is the display's refresh interval; a longer entry is a frame that
    /// reached the screen late.
    const FrameTimes &frame_times() const;
    /// The view used by all drawing calls. poll() sets its viewport; the application owns
    /// position and zoom.
    Camera2D camera;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color color);
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc);
};

} // namespace kinetra
