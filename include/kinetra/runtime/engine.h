#pragma once

#include <array>
#include <cstdint>
#include <kinetra/view/camera.h>
#include <kinetra/view/input.h>
#include <kinetra/view/timing.h>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace kinetra {

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

/// Immutable, CPU-side RGBA image. Supply bytes by row from the top left, four bytes per
/// pixel with straight alpha and no row padding. The image stores premultiplied pixels.
class RgbaImage {
  public:
    /// Copies and premultiplies the pixels. Throws std::invalid_argument for nonpositive dimensions or
    /// a byte count other than width * height * 4.
    RgbaImage(int width, int height, std::span<const std::uint8_t> pixels);
    RgbaImage(const RgbaImage &) = delete;
    RgbaImage &operator=(const RgbaImage &) = delete;
    /// Width in pixels.
    int width() const { return width_; }
    /// Height in pixels.
    int height() const { return height_; }

  private:
    friend class Runtime;
    int width_, height_;
    std::vector<std::uint8_t> pixels_;
};

/// SDL2/OpenGL 3.3 window, input and 2D drawing. Own on the main
/// thread; only one Runtime may exist at a time. Throws runtime_error on setup,
/// shader or rendering initialization failures. No simulation dependency.
///
/// A frame is poll(), then begin(), any number of drawing calls, and end(). Drawing calls
/// take world coordinates and go through `camera`. Shapes fill a batch, which a textured
/// draw or end() flushes in call order.
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
    /// Draws `image` in a world-space rectangle starting at `top_left`, with positive `size`.
    /// Call between begin() and end(). The runtime reuses the last upload without keeping
    /// the CPU image alive. Throws std::invalid_argument for a null image, invalid bounds
    /// or dimensions exceeding the context's texture limit, and std::runtime_error
    /// if the image shader or texture cannot be prepared.
    void image(const std::shared_ptr<const RgbaImage> &image, Vec2 top_left, Vec2 size);
    /// Uploads and draws the batch, then presents the frame. Without VSync it then waits for
    /// the frame limiter, unless that was switched off (see pacing()). From the second frame
    /// on, also records the time since the previous end() in frame_times(), or the fixed time
    /// set by fix_frame_times().
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
    /// reached the screen late. After fix_frame_times(seconds), every new entry is `seconds`.
    const FrameTimes &frame_times() const;
    /// Makes end() record `seconds` in frame_times() for every frame instead of the measured
    /// time, or measure again for std::nullopt, the default. For a program that renders frames
    /// into files: each of its frames stands for one display refresh, not for the time it took
    /// to draw, and a measured graph would differ on every run. Pacing does not change; entries
    /// already recorded stay. Throws std::invalid_argument for a negative or non-finite time.
    void fix_frame_times(std::optional<double> seconds);
    /// The view used by all drawing calls. poll() sets its viewport; the application owns
    /// position and zoom.
    Camera2D camera;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    Vec2 clip_point(Vec2 world) const;
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color color);
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc);
};

} // namespace kinetra
