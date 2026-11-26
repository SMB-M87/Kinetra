// Graphics tests for the SDL2/OpenGL runtime and the session and launcher loops. They
// open real windows, so they run only with KINETRA_TEST_GRAPHICS. Only one Runtime may
// exist at a time; each case owns its own and closes it before the next case starts.
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <kinetra/runtime/debug_view.h>
#include <kinetra/runtime/engine.h>
#include <kinetra/runtime/inspector_view.h>
#include <kinetra/runtime/launcher.h>
#include <kinetra/runtime/session.h>
#include <kinetra/runtime/settings_view.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace kinetra;
namespace {
void send(SDL_Event event) {
    if (SDL_PushEvent(&event) < 0)
        throw std::runtime_error(SDL_GetError());
}
void press(SDL_Keycode code) {
    SDL_Event event{};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = code;
    send(event);
}

/// Session that records its calls and raises `request` from advance number `request_at` on.
struct ScriptedSession final : Session {
    int advances{}, renders{};
    int request_at{0};
    SessionRequest raised{SessionRequest::none};
    bool escape_closes{true};
    int *destroyed{};
    int *rendered{};
    /// Called once, in the advance that raises the request.
    std::function<void()> on_request;
    ScriptedSession(SessionRequest request, int at, bool escape, int *destroyed_count = nullptr,
                    int *render_count = nullptr)
        : request_at(at), raised(request), escape_closes(escape), destroyed(destroyed_count),
          rendered(render_count) {}
    ~ScriptedSession() override {
        if (destroyed)
            ++*destroyed;
    }
    void advance(double elapsed, const Input &, Camera2D &) override {
        REQUIRE_MESSAGE(elapsed >= 0, "Negative frame time");
        ++advances;
        if (advances == request_at && on_request)
            on_request();
    }
    void render(Runtime &runtime) override {
        ++renders;
        if (rendered)
            ++*rendered;
        runtime.begin();
        runtime.end();
    }
    std::string title() override { return "SCRIPTED SESSION"; }
    SessionRequest request() const override {
        return request_at > 0 && advances >= request_at ? raised : SessionRequest::none;
    }
    bool close_on_escape() const override { return escape_closes; }
};

/// Launcher registration whose factory hands out one ScriptedSession and records it.
struct ScriptedApplication {
    SessionRequest request{SessionRequest::none};
    int request_at{0};
    bool escape_closes{true};
    /// Configuration names of the registration; none by default.
    std::vector<std::string> configurations;
    int created{}, destroyed{}, renders{};
    /// The configuration index of every session created, in order.
    std::vector<std::size_t> started;
    /// Handed to every session as ScriptedSession::on_request.
    std::function<void()> on_request;
    std::vector<ApplicationRegistration> registrations() {
        return {{{"SCRIPTED", "TEST", configurations, 0},
                 [this](Camera2D &, std::size_t configuration) -> std::unique_ptr<Session> {
                     ++created;
                     started.push_back(configuration);
                     auto session = std::make_unique<ScriptedSession>(request, request_at, escape_closes,
                                                                      &destroyed, &renders);
                     session->on_request = on_request;
                     return session;
                 }}};
    }
};
} // namespace

TEST_CASE("input drawing and resizing reach SDL and OpenGL") {
    Runtime runtime("Kinetra graphics test", 320, 240);
    runtime.read_back(true);
    runtime.poll();
    SDL_Event event{};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_w;
    send(event);
    runtime.poll();
    REQUIRE_MESSAGE((runtime.input().down(Key::w) && runtime.input().hit(Key::w)), "Key press lost");
    runtime.poll();
    REQUIRE_MESSAGE((runtime.input().down(Key::w) && !runtime.input().hit(Key::w)),
                    "Key transition repeated");
    event.type = SDL_KEYUP;
    send(event);
    runtime.poll();
    REQUIRE_MESSAGE(!runtime.input().down(Key::w), "Key release lost");
    for (const auto &[code, key] :
         {std::pair{SDLK_f, Key::f},           std::pair{SDLK_p, Key::p},
          std::pair{SDLK_h, Key::h},           std::pair{SDLK_t, Key::t},
          std::pair{SDLK_c, Key::c},           std::pair{SDLK_RETURN, Key::enter},
          std::pair{SDLK_m, Key::m},           std::pair{SDLK_LSHIFT, Key::shift},
          std::pair{SDLK_LCTRL, Key::control}, std::pair{SDLK_RCTRL, Key::control},
          std::pair{SDLK_v, Key::v},           std::pair{SDLK_BACKQUOTE, Key::backquote},
          std::pair{SDLK_1, Key::one},         std::pair{SDLK_2, Key::two},
          std::pair{SDLK_3, Key::three},       std::pair{SDLK_4, Key::four},
          std::pair{SDLK_5, Key::five},        std::pair{SDLK_6, Key::six},
          std::pair{SDLK_UP, Key::up},         std::pair{SDLK_DOWN, Key::down},
          std::pair{SDLK_LEFT, Key::left},     std::pair{SDLK_RIGHT, Key::right}}) {
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = code;
        send(event);
        runtime.poll();
        REQUIRE_MESSAGE((runtime.input().down(key) && runtime.input().hit(key)), "Sandbox key press lost");
        event.type = SDL_KEYUP;
        send(event);
        runtime.poll();
        REQUIRE_MESSAGE(!runtime.input().down(key), "Sandbox key release lost");
    }
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_a;
    send(event);
    event = {};
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    send(event);
    runtime.poll();
    REQUIRE_MESSAGE(!runtime.input().down(Key::a), "Focus loss retained key");
    // Fill the mouse events as SDL fills real ones, with whole notches in `y` and the exact
    // delta in `preciseY`. sdl2-compat (Arch, WSL2) returns a pushed wheel event with `y`
    // replaced by its `mouseY` (0 here) and `preciseY` set from `y`, so this check fails
    // there when poll() reads `y`. A fractional step cannot be pushed through sdl2-compat.
    event = {};
    event.type = SDL_MOUSEWHEEL;
    event.wheel.y = 2;
    event.wheel.preciseY = 2.f;
    send(event);
    event = {};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.state = SDL_PRESSED;
    event.button.clicks = 1;
    send(event);
    runtime.poll();
    REQUIRE_MESSAGE(runtime.input().clicked, "Mouse click lost");
    REQUIRE_MESSAGE(runtime.input().wheel == 2, "Mouse wheel lost");
    runtime.poll();
    REQUIRE_MESSAGE((!runtime.input().clicked && runtime.input().mouse_down && runtime.input().wheel == 0),
                    "Held shooting input or mouse transitions lost");
    event = {};
    event.type = SDL_MOUSEWHEEL;
    event.wheel.y = 1;
    event.wheel.preciseY = 1.f;
    event.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
    send(event);
    runtime.poll();
    REQUIRE_MESSAGE(runtime.input().wheel == -1, "Flipped wheel step lost");
    event = {};
    event.type = SDL_MOUSEBUTTONUP;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.state = SDL_RELEASED;
    event.button.clicks = 1;
    send(event);
    runtime.poll();
    REQUIRE_MESSAGE(!runtime.input().mouse_down, "Mouse release retained shooting");
    runtime.begin({0, 0, 0, 1});
    runtime.box({0, 0}, {2, 2}, 0.3f, {0, 1, 0, 1});
    runtime.circle({4, 0}, 1, {1, 0, 0, 1});
    runtime.line({-4, -1}, {-4, 1}, 0.1f, {0, 0, 1, 1});
    runtime.end();
    // The pixel just above the window's center, as OpenGL's bottom-up row 120 was.
    auto pixels = runtime.frame().pixel(160, 119);
    REQUIRE_MESSAGE((pixels[1] > 200 && pixels[0] < 20 && pixels[2] < 20), "Rendered center pixel missing");
    REQUIRE_MESSAGE(glGetError() == GL_NO_ERROR, "OpenGL reported an error");
    runtime.begin({0, 0, 0, 1});
    const Vec2 polygon[]{{-2, -2}, {2, -2}, {0, 2}};
    runtime.polygon(polygon, {0, 1, 0});
    runtime.end();
    pixels = runtime.frame().pixel(160, 119);
    REQUIRE_MESSAGE((pixels[1] > 200 && pixels[0] < 20), "Filled polygon missing from rendered batch");
    for (const float zoom : {2.5f, 50.f}) {
        runtime.camera.pixels_per_unit = zoom;
        runtime.begin({0, 0, 0, 1});
        draw_debug_vector(runtime, {}, {0.0001f, 0}, {0, 1, 0});
        runtime.end();
        pixels = runtime.frame().pixel(168, 119);
        REQUIRE_MESSAGE(pixels[1] > 200, "Small debug vector vanished at camera zoom");
    }
    // Read a full circle back to verify both coverage and a fractional-alpha edge.
    runtime.camera.pixels_per_unit = 50;
    runtime.begin({0, 0, 0, 1});
    runtime.circle({}, 1.73f, {0, 1, 0, 1});
    runtime.end();
    // Rows from the top; the circle is symmetric about the window's center, so the distances
    // below are the same as with OpenGL's bottom-up rows.
    const auto &image = runtime.frame().rgba;
    REQUIRE(image.size() == std::size_t(320 * 240 * 4));
    int softened = 0;
    for (int y = 30; y < 210; ++y)
        for (int x = 70; x < 250; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - 160, dy = static_cast<float>(y) + 0.5f - 120;
            const float distance = std::sqrt(dx * dx + dy * dy);
            const auto green = image[(y * 320 + x) * 4 + 1];
            if (distance < 85.5f)
                REQUIRE_MESSAGE(green > 240, "Circle polygon left an interior gap");
            if (green > 10 && green < 240)
                ++softened;
        }
    REQUIRE_MESSAGE(softened > 100, "Circle edge has no antialiasing coverage");
    runtime.camera.pixels_per_unit = 20;
    SDL_Window *window = SDL_GL_GetCurrentWindow();
    REQUIRE_MESSAGE((window && (SDL_GetWindowFlags(window) & SDL_WINDOW_RESIZABLE)),
                    "Window must support resizing");
    // Resize the real window in both directions; synthetic events alone
    // would not exercise the logical size and drawable size queries.
    for (const auto size : {Vec2{640, 360}, Vec2{200, 150}}) {
        SDL_SetWindowSize(window, int(size.x), int(size.y));
        runtime.poll();
        REQUIRE_MESSAGE(runtime.camera.viewport == size, "Camera did not follow window resize");
        const Vec2 center = runtime.camera.world_to_screen(runtime.camera.position);
        REQUIRE_MESSAGE(center == size / 2, "Camera center moved after resize");
        const Vec2 picked = runtime.camera.screen_to_world(center + Vec2{20, -20});
        REQUIRE_MESSAGE((picked == runtime.camera.position + Vec2{1, 1}),
                        "Picking scale changed after resize");
        runtime.begin({0, 0, 0, 1});
        runtime.box(runtime.camera.position, {2, 2}, 0, {0, 1, 0, 1});
        runtime.end();
        int width, height;
        SDL_GL_GetDrawableSize(window, &width, &height);
        GLint viewport[4]{};
        glGetIntegerv(GL_VIEWPORT, viewport);
        REQUIRE_MESSAGE((viewport[2] == width && viewport[3] == height),
                        "OpenGL viewport did not follow drawable resize");
        const auto &frame = runtime.frame();
        REQUIRE_MESSAGE((frame.width == width && frame.height == height),
                        "Read-back frame did not follow drawable resize");
        // The pixel just above and left of the center. SDL's offscreen driver keeps the
        // surface of the window's first size, so the frame beyond 320 x 240 is undefined there,
        // and the center of the 640 x 360 window lies just outside it.
        pixels = frame.pixel(width / 2 - 1, height / 2 - 1);
        REQUIRE_MESSAGE((pixels[1] > 200 && pixels[0] < 20 && pixels[2] < 20),
                        "Rendered center pixel missing after resize");
        REQUIRE_MESSAGE(glGetError() == GL_NO_ERROR, "OpenGL error after resize");
    }
    event = {};
    event.type = SDL_QUIT;
    send(event);
    REQUIRE_MESSAGE(!runtime.poll(), "Window quit ignored");
}

TEST_CASE("read-back frames hold the drawn pixels from the top left until switched off") {
    Runtime runtime("Kinetra read-back", 320, 240);
    REQUIRE(runtime.poll());
    CHECK_THROWS_AS(runtime.frame(), std::logic_error);
    runtime.begin({0, 0, 0, 1});
    runtime.end();
    CHECK_THROWS_AS(runtime.frame(), std::logic_error); // Frames before read_back(true) are not kept.
    runtime.read_back(true);
    CHECK_THROWS_AS(runtime.frame(), std::logic_error); // Switched on, but no end() since.
    // A red box in the top-left quarter and a translucent one in the bottom-right, on blue.
    const auto world = [&](Vec2 point) { return runtime.camera.screen_to_world(point); };
    const auto units = runtime.camera.pixels_per_unit;
    runtime.begin({0, 0, 1, 1});
    runtime.box(world({40, 30}), Vec2{20, 20} / units, 0, {1, 0, 0, 1});
    runtime.box(world({280, 210}), Vec2{20, 20} / units, 0, {0, 1, 0, 0.5f});
    runtime.end();
    const auto &frame = runtime.frame();
    int width, height;
    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &width, &height);
    CHECK(frame.width == width);
    CHECK(frame.height == height);
    REQUIRE(frame.rgba.size() == std::size_t(width) * std::size_t(height) * 4);
    using Rgba = std::array<std::uint8_t, 4>;
    const auto at = [&](Vec2 point) {
        return frame.pixel(int(point.x * float(width) / runtime.camera.viewport.x),
                           int(point.y * float(height) / runtime.camera.viewport.y));
    };
    CHECK_MESSAGE((at({40, 30}) == Rgba{255, 0, 0, 255}), "rows start at the top");
    CHECK_MESSAGE((at({40, 210}) == Rgba{0, 0, 255, 255}), "bottom-left is background");
    const auto blended = at({280, 210});
    CHECK(blended[1] >= 125);
    CHECK(blended[1] <= 130);
    CHECK(blended[2] >= 125);
    CHECK(blended[2] <= 130);
    CHECK_MESSAGE(blended[3] == 255, "the window is opaque, whatever blending did to alpha");
    CHECK_THROWS_AS(frame.pixel(-1, 0), std::out_of_range);
    CHECK_THROWS_AS(frame.pixel(0, height), std::out_of_range);
    CHECK_THROWS_AS(frame.pixel(width, 0), std::out_of_range);
    // The next read-back frame replaces the stored one in place.
    runtime.begin({0, 1, 0, 1});
    runtime.end();
    CHECK(&runtime.frame() == &frame);
    CHECK((at({40, 30}) == Rgba{0, 255, 0, 255}));
    runtime.read_back(true); // Already on: the frame stays.
    CHECK_NOTHROW(runtime.frame());
    CHECK((at({40, 30}) == Rgba{0, 255, 0, 255}));
    runtime.read_back(false);
    CHECK_THROWS_AS(runtime.frame(), std::logic_error);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("read-back ignores the caller's pack state and restores it") {
    // An odd width: at an alignment of 8, OpenGL would pad each 4 * 161-byte row to 648.
    Runtime runtime("Kinetra read-back pack state", 161, 121);
    REQUIRE(runtime.poll());
    runtime.read_back(true);
    // State a caller may leave behind. Each piece alone would make glReadPixels overrun its
    // destination, shift the rows, or write into the pack buffer instead of the frame.
    GLuint pack_buffer{};
    glGenBuffers(1, &pack_buffer);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pack_buffer);
    glBufferData(GL_PIXEL_PACK_BUFFER, 1 << 20, nullptr, GL_STREAM_READ);
    const std::array<std::pair<GLenum, GLint>, 4> pack{{{GL_PACK_ALIGNMENT, 8},
                                                        {GL_PACK_ROW_LENGTH, 300},
                                                        {GL_PACK_SKIP_ROWS, 3},
                                                        {GL_PACK_SKIP_PIXELS, 5}}};
    for (const auto &[name, value] : pack)
        glPixelStorei(name, value);
    glReadBuffer(GL_FRONT);
    // A red box at the top-left corner, on blue: a shifted read would move it.
    const auto units = runtime.camera.pixels_per_unit;
    runtime.begin({0, 0, 1, 1});
    runtime.box(runtime.camera.screen_to_world({10, 10}), Vec2{10, 10} / units, 0, {1, 0, 0, 1});
    runtime.end();
    const auto &frame = runtime.frame();
    REQUIRE(frame.rgba.size() == std::size_t(frame.width) * std::size_t(frame.height) * 4);
    const auto at = [&](Vec2 point) {
        return frame.pixel(int(point.x * float(frame.width) / runtime.camera.viewport.x),
                           int(point.y * float(frame.height) / runtime.camera.viewport.y));
    };
    CHECK(at({0, 0})[0] == 255);
    CHECK(at({10, 19.5f})[0] == 255);
    CHECK(at({10, 25})[2] == 255);
    CHECK(at({25, 10})[2] == 255);
    CHECK(at({160, 120})[2] == 255);
    for (const auto &[name, value] : pack) {
        GLint now{};
        glGetIntegerv(name, &now);
        CAPTURE(name);
        CHECK_MESSAGE(now == value, "read-back must leave the caller's pack state");
        glPixelStorei(name, name == GL_PACK_ALIGNMENT ? 4 : 0);
    }
    GLint bound{}, read_buffer{};
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &bound);
    glGetIntegerv(GL_READ_BUFFER, &read_buffer);
    CHECK_MESSAGE(GLuint(bound) == pack_buffer, "read-back must leave the caller's pack buffer bound");
    CHECK_MESSAGE(read_buffer == GL_FRONT, "read-back must leave the caller's read buffer");
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glDeleteBuffers(1, &pack_buffer);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("presented frames are timed and the frame-time graph draws them") {
    Runtime runtime("Kinetra frame times", 320, 200);
    runtime.read_back(true);
    REQUIRE(runtime.poll());
    for (int frame = 0; frame < 3; ++frame) {
        runtime.begin({0, 0, 0, 1});
        runtime.end();
    }
    REQUIRE_MESSAGE(runtime.frame_times().size() == 2, "The first frame has no predecessor to time");
    CHECK(runtime.frame_times().worst() >= 0);
    // A fixed history makes the bars predictable: every frame within one 60 Hz refresh.
    FrameTimes times;
    for (std::size_t i = 0; i < FrameTimes::capacity; ++i)
        times.record(0.010);
    runtime.begin({0, 0, 0, 1});
    draw_frame_times(runtime, times);
    runtime.end();
    // Read the bar area's bottom row, just above the bars' base, from the presented frame.
    const Vec2 origin = FrameTimeGraph::origin(runtime.camera.viewport);
    const int row = int(std::ceil(origin.y + FrameTimeGraph::height - 2)) - 1;
    int green = 0;
    for (int x = int(origin.x); x < int(origin.x) + 240; ++x) {
        const auto pixel = runtime.frame().pixel(x, row);
        if (pixel[0] < 120 && pixel[1] > 180 && pixel[2] < 140)
            ++green;
    }
    CHECK_MESSAGE(green > 200, "Short frames were not drawn as green bars");
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("a fixed frame time replaces the measured one until it is cleared") {
    Runtime runtime("Kinetra fixed frame times", 160, 120);
    REQUIRE(runtime.poll());
    CHECK_THROWS_AS(runtime.fix_frame_times(-0.001), std::invalid_argument);
    CHECK_THROWS_AS(runtime.fix_frame_times(std::numeric_limits<double>::infinity()), std::invalid_argument);
    CHECK_THROWS_AS(runtime.fix_frame_times(std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
    // Longer than any empty frame takes, so a measured frame cannot be mistaken for it.
    constexpr double fixed = 1;
    runtime.fix_frame_times(fixed);
    for (int frame = 0; frame < 4; ++frame) {
        runtime.begin({0, 0, 0, 1});
        runtime.end();
    }
    REQUIRE_MESSAGE(runtime.frame_times().size() == 3, "The first frame has no predecessor to time");
    for (std::size_t i = 0; i < runtime.frame_times().size(); ++i)
        CHECK(runtime.frame_times()[i] == fixed);
    runtime.fix_frame_times(std::nullopt);
    runtime.begin({0, 0, 0, 1});
    runtime.end();
    REQUIRE(runtime.frame_times().size() == 4);
    CHECK_MESSAGE(runtime.frame_times()[3] < fixed, "Cleared, the frame is measured again");
    CHECK_MESSAGE(runtime.frame_times()[0] == fixed, "Entries recorded before stay");
}

namespace {
/// Whether the frame the last end() read back shows `expected` at logical screen pixel
/// `point` (Y down), within rounding. The case must have called read_back(true).
bool presented(const Runtime &runtime, Vec2 point, Color expected) {
    const auto &frame = runtime.frame();
    const auto pixel = frame.pixel(int(point.x * float(frame.width) / runtime.camera.viewport.x),
                                   int(point.y * float(frame.height) / runtime.camera.viewport.y));
    const auto near = [](std::uint8_t channel, float value) {
        return std::abs(float(channel) - value * 255) <= 3;
    };
    return near(pixel[0], expected.r) && near(pixel[1], expected.g) && near(pixel[2], expected.b);
}
} // namespace

TEST_CASE("RGBA images keep their row order and blend between ordinary shapes") {
    const std::uint8_t pixels[]{255, 0, 0, 255, 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 0, 0};
    CHECK_THROWS_AS(RgbaImage(0, 2, pixels), std::invalid_argument);
    CHECK_THROWS_AS(RgbaImage(2, 3, pixels), std::invalid_argument);
    const auto image = std::make_shared<const RgbaImage>(2, 2, pixels);
    Runtime runtime("RGBA images", 320, 240);
    runtime.read_back(true);
    runtime.camera.position = {4, -9};
    const auto world = [&](Vec2 point) { return runtime.camera.screen_to_world(point); };
    const auto units = runtime.camera.pixels_per_unit;
    runtime.begin();
    runtime.box(world({160, 120}), Vec2{100, 100} / units, 0, {0, 0, 1});
    runtime.image(image, world({60, 20}), Vec2{200, 200} / units);
    runtime.box(world({230, 50}), Vec2{10, 10} / units, 0, {1, 1, 1, 0.5f});
    runtime.end();
    CHECK(presented(runtime, {90, 50}, {1, 0, 0}));
    CHECK(presented(runtime, {90, 190}, {0, 1, 0}));
    CHECK(presented(runtime, {160, 190}, {0, 0.5f, 0.5f})); // Filter the opaque/transparent boundary.
    CHECK(presented(runtime, {230, 190}, {0, 0, 1}));
    CHECK(presented(runtime, {230, 50}, {1, 0.5f, 0.5f}));
}

TEST_CASE("a pending OpenGL error leaves the previous image usable") {
    const std::uint8_t red[]{255, 0, 0, 255}, green[]{0, 255, 0, 255};
    const auto first = std::make_shared<const RgbaImage>(1, 1, red);
    const auto second = std::make_shared<const RgbaImage>(1, 1, green);
    Runtime runtime("Texture upload recovery", 320, 240);
    runtime.read_back(true);
    const auto corner = runtime.camera.screen_to_world({0, 0});
    const auto size = runtime.camera.viewport / runtime.camera.pixels_per_unit;
    runtime.begin();
    runtime.image(first, corner, size);
    runtime.end();
    runtime.begin();
    glEnable(GL_TEXTURE_2D); // This fixed-function state is invalid in the core profile.
    CHECK_THROWS_AS(runtime.image(second, corner, size), std::runtime_error);
    runtime.image(first, corner, size);
    runtime.end();
    CHECK(presented(runtime, {160, 120}, {1, 0, 0}));
    runtime.begin();
    runtime.image(second, corner, size);
    runtime.end();
    CHECK(presented(runtime, {160, 120}, {0, 1, 0}));
}

TEST_CASE("the drawn preview and its description follow the highlighted application") {
    const std::uint8_t red[]{255, 0, 0, 255}, green[]{0, 255, 0, 255};
    LaunchMenu menu({{"FIRST", "FIRST", {}, 0, std::make_shared<const RgbaImage>(1, 1, red)},
                     {"SECOND", "I AM NEXT", {}, 0, std::make_shared<const RgbaImage>(1, 1, green)}});
    Runtime runtime("Launcher previews", 1280, 800);
    runtime.read_back(true);
    const auto layout = menu.layout(runtime.camera.viewport);
    REQUIRE(layout.preview_origin);
    const auto image_center = *layout.preview_origin + Vec2{135, 80} * layout.scale;
    const auto caption_start = *layout.preview_origin + Vec2{10.5f, 160.5f} * layout.scale;
    const auto draw = [&] {
        menu.draw(runtime);
        menu.draw(runtime);
    };
    draw();
    CHECK(presented(runtime, image_center, {1, 0, 0}));
    CHECK(presented(runtime, caption_start, {0.85f, 0.9f, 1})); // First pixel of F.
    Input input;
    input.mouse = layout.row_center(1);
    CHECK_FALSE(menu.update(input, runtime.camera.viewport));
    draw();
    CHECK(presented(runtime, image_center, {0, 1, 0}));
    CHECK(presented(runtime, caption_start, {0.06f, 0.1f, 0.16f})); // First pixel of I is blank.
    input.pressed[std::size_t(Key::up)] = true;
    CHECK_FALSE(menu.update(input, runtime.camera.viewport));
    draw();
    CHECK(presented(runtime, image_center, {1, 0, 0}));
    CHECK(presented(runtime, caption_start, {0.85f, 0.9f, 1}));
}

TEST_CASE("the inspector scrolls wrapped values while its close button stays in place") {
    Runtime runtime("Kinetra inspector", 640, 480);
    runtime.read_back(true);
    InspectorView view;
    const Aabb area{{340, 8}, {632, 65}};
    const InspectorSnapshot snapshot{
        "MOVER", {{"DETAILS", {{"FIRST", std::string(96, 'A')}, {"LAST", "LAST VALUE"}}}}};
    const auto box = view.bounds(snapshot, area);
    CHECK(box.max.x == area.max.x);
    CHECK(box.min.y == area.min.y);
    CHECK(box.max.y <= area.max.y);
    Input outside;
    outside.mouse = {box.min.x - 1, box.min.y + 10};
    outside.wheel = -10;
    CHECK_FALSE(view.update(outside, snapshot, area).captured);
    const auto draw = [&] {
        runtime.begin({0, 0, 0});
        view.draw(runtime, snapshot, area);
        runtime.end();
    };
    draw();
    CHECK(presented(runtime, {box.max.x - 12, box.min.y + 6}, overlay::button_color));
    CHECK(presented(runtime, {box.min.x + 8, box.min.y + 28}, overlay::bright_color));
    Input scroll;
    scroll.mouse = {box.max.x - 10, box.max.y - 1};
    scroll.wheel = -1;
    CHECK(view.update(scroll, snapshot, area).captured);
    draw();
    // The first visible line here is a continuation of the long value, without its label.
    CHECK(presented(runtime, {box.min.x + 8, box.min.y + 28}, overlay::panel_color));
    CHECK(presented(runtime, {box.min.x + 153, box.min.y + 28}, overlay::value_color));
    scroll.wheel = -100;
    CHECK(view.update(scroll, snapshot, area).captured);
    draw();
    CHECK(presented(runtime, {box.min.x + 8, box.min.y + 28}, overlay::text_color));

    const Aabb resized{{340, 12}, {632, 96}};
    const auto larger = view.bounds(snapshot, resized);
    runtime.begin({0, 0, 0});
    view.draw(runtime, snapshot, resized);
    runtime.end();
    // Four rows now fit. They start with a continuation and still end at LAST.
    CHECK(presented(runtime, {larger.min.x + 8, larger.min.y + 28}, overlay::panel_color));
    CHECK(presented(runtime, {larger.min.x + 153, larger.min.y + 28}, overlay::value_color));
    CHECK(presented(runtime, {larger.min.x + 8, larger.min.y + 67}, overlay::text_color));
    Input close;
    close.mouse = {larger.max.x - 12, larger.min.y + 12};
    close.clicked = true;
    const auto result = view.update(close, snapshot, resized);
    CHECK(result.captured);
    CHECK(result.close);

    const InspectorSnapshot replacement{"NEXT MOVER", {{"DETAILS", {{"SHORT", "ONE VALUE"}}}}};
    view.reset();
    runtime.begin({0, 0, 0});
    view.draw(runtime, replacement, area);
    runtime.end();
    CHECK(presented(runtime, {box.min.x + 8, box.min.y + 28}, overlay::bright_color));
}

TEST_CASE("the debug menu and inspector stay interactive with the graph below the inspector") {
    Runtime runtime("Kinetra inspector and frame times", 640, 480);
    runtime.read_back(true);
    DebugMenu menu;
    menu.visible = true;
    menu.screen_origin = {8, 32};
    bool running = true;
    const auto entries = std::array{debug_check("RUNNING", running)};
    InspectorView inspector;
    const InspectorSnapshot snapshot{"MOVER", {{"STATE", {{"SPEED", "0 MM/TICK"}}}}};
    const Aabb area{{340, 8}, {632, 300}};
    const auto box = inspector.bounds(snapshot, area);
    const Vec2 graph{FrameTimeGraph::origin(runtime.camera.viewport).x, box.max.y + 12};
    constexpr float graph_scale = 0.75f;
    FrameTimes times;
    for (std::size_t i = 0; i < FrameTimes::capacity; ++i)
        times.record(0.010);
    runtime.begin({0, 0, 0});
    menu.draw(runtime, entries);
    inspector.draw(runtime, snapshot, area);
    draw_frame_times(runtime, times, graph, graph_scale);
    runtime.end();
    CHECK(presented(runtime, {box.max.x - 12, box.min.y + 6}, overlay::button_color));
    CHECK(presented(runtime, graph + Vec2{8, FrameTimeGraph::height - 3} * graph_scale, {0.3f, 0.9f, 0.4f}));
    CHECK(presented(runtime, graph + Vec2{0, FrameTimeGraph::height + 4} * graph_scale, {1, 1, 1}));

    Input click;
    click.clicked = true;
    click.mouse =
        menu.screen_origin + Vec2{16, DebugMenu::row_top(0) + 8} * DebugMenu::scale(runtime.camera.viewport);
    CHECK_FALSE(inspector.update(click, snapshot, area).captured);
    CHECK(menu.update(click, runtime.camera.viewport, entries));
    CHECK_FALSE(running);
    click.mouse = {box.max.x - 12, box.min.y + 12};
    CHECK_FALSE(menu.update(click, runtime.camera.viewport, entries));
    CHECK(inspector.update(click, snapshot, area).close);
}

TEST_CASE("the debug menu draws its panel, section band, marks and swatch") {
    Runtime runtime("Kinetra debug menu", 640, 480);
    runtime.read_back(true);
    REQUIRE(runtime.poll());
    bool flag = true;
    float speed = 2;
    DebugFolds folds("MOTION");
    const std::vector<DebugEntry> entries{
        folds.section("MOTION"), debug_check("RECOVERY", flag), debug_number("SPEED", speed, {1, 4, 1}),
        DebugEntry{.label = "RESET", .action = [] {}, .swatch = Color{1, 0, 0}, .rule_above = true}};
    DebugMenu menu;
    menu.visible = true;
    menu.screen_origin = {8, 32};
    runtime.begin({0, 0, 0, 1});
    menu.draw(runtime, entries);
    runtime.end();
    const float scale = DebugMenu::scale(runtime.camera.viewport);
    // A point of the panel, given in panel pixels.
    const auto at = [&](float x, float y) { return menu.screen_origin + Vec2{x, y} * scale; };
    CHECK_MESSAGE(presented(runtime, at(100, 3), overlay::title_color), "title bar");
    CHECK_MESSAGE(presented(runtime, at(120, DebugMenu::row_top(0) + 8), overlay::section_color),
                  "band of the section heading");
    CHECK_MESSAGE(presented(runtime, at(120, DebugMenu::row_top(1) + 8), overlay::panel_color), "panel body");
    CHECK_MESSAGE(presented(runtime,
                            at(DebugMenu::label_left(1) - DebugMenu::mark_width + 2.5f,
                               DebugMenu::row_top(1) + DebugMenu::text_top + 3.5f),
                            overlay::on_color),
                  "a check box that is on is filled");
    CHECK_MESSAGE(presented(runtime, at(DebugMenu::increase_left + 1.5f, DebugMenu::row_top(2) + 4),
                            overlay::button_color),
                  "the [+] button of a number row");
    CHECK_MESSAGE(presented(runtime,
                            at(DebugMenu::panel_width - DebugMenu::text_margin - 3.5f,
                               DebugMenu::row_top(3) + DebugMenu::text_top + 3.5f),
                            {1, 0, 0}),
                  "swatch at the right margin");
    CHECK_MESSAGE(presented(runtime, at(DebugMenu::panel_width + 6, 40), {0, 0, 0}),
                  "nothing beside the panel");
    // The row under the pointer lights up when a click would do something.
    menu.pointer = at(120, DebugMenu::row_top(3) + 8);
    runtime.begin({0, 0, 0, 1});
    menu.draw(runtime, entries);
    runtime.end();
    CHECK_MESSAGE(presented(runtime, at(120, DebugMenu::row_top(3) + 8), overlay::band_color), "hovered row");
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("the settings panel and its toolbar draw in the shared style") {
    Runtime runtime("Kinetra settings", 640, 480);
    runtime.read_back(true);
    REQUIRE(runtime.poll());
    bool statistics = true, debug = true;
    SettingsOverlay overlay({ToggleRow{"STATISTICS", &statistics}, ToggleRow{"DEBUG MODE", &debug},
                             HelpSection{"CONTROLS", {"WASD  MOVE"}}},
                            &debug);
    runtime.begin({0, 0, 0, 1});
    overlay.draw(runtime);
    runtime.end();
    CHECK_MESSAGE(presented(runtime, {6, 6}, overlay::button_color), "SETTINGS toolbar button");
    CHECK_MESSAGE(presented(runtime, {70, 6}, overlay::button_color), "DEBUG toolbar button in debug mode");
    Input open;
    open.pressed[std::size_t(Key::control)] = true;
    open.mouse = {630, 470};
    REQUIRE(overlay.update(open, runtime.camera));
    runtime.begin({0, 0, 0, 1});
    overlay.draw(runtime);
    runtime.end();
    const float scale = SettingsOverlay::scale(runtime.camera.viewport);
    const Vec2 origin = runtime.camera.viewport / 2 - overlay.half_extent() * scale;
    const auto at = [&](float x, float y) { return origin + Vec2{x, y} * scale; };
    CHECK_MESSAGE(presented(runtime, at(100, 3), overlay::title_color), "title bar");
    CHECK_MESSAGE(presented(runtime, at(150, overlay.row_y(0) + 3), overlay::panel_color), "panel body");
    CHECK_MESSAGE(presented(runtime, at(14.5f, overlay.row_y(0) + 3.5f), overlay::on_color),
                  "a toggle that is on is a filled box");
    CHECK_MESSAGE(presented(runtime, at(SettingsOverlay::resume_x - 26, overlay.actions_y() + 3),
                            overlay::button_color),
                  "RESUME button");
    CHECK_MESSAGE(presented(runtime, {6, 6}, {0, 0, 0}), "the toolbar is hidden while the panel is open");
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("without vsync the frame limiter spaces frames by the display's refresh interval") {
    Runtime runtime("Kinetra pacing", 320, 200);
    REQUIRE(runtime.poll());
    CHECK(runtime.frame_interval() > 0);
    if (runtime.pacing() == FramePacing::vsync) {
        runtime.limit_frames(false);
        CHECK_MESSAGE(runtime.pacing() == FramePacing::vsync, "the limiter has no say under VSync");
        return;
    }
    CHECK_MESSAGE(runtime.pacing() == FramePacing::limited, "the limiter is on by default");
    constexpr int frames = 24;
    for (int frame = 0; frame < frames; ++frame) {
        runtime.begin({0, 0, 0, 1});
        runtime.end();
    }
    // The first timed frame may start the limiter's series; every later one waits for its
    // deadline, so the frames cannot average less than one interval.
    CHECK(runtime.frame_times().size() == frames - 1);
    CHECK(runtime.frame_times().average() >= runtime.frame_interval() * 0.9);
    runtime.limit_frames(false);
    CHECK(runtime.pacing() == FramePacing::uncapped);
    runtime.limit_frames(true);
    CHECK(runtime.pacing() == FramePacing::limited);
    runtime.begin({0, 0, 0, 1});
    draw_frame_times(runtime, runtime.frame_times());
    runtime.end();
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("a runtime reopens after destruction") {
    Runtime reopened("Kinetra reopened", 160, 120);
    CHECK(reopened.poll());
}

TEST_CASE("run_session honors the frame limit") {
    Runtime runtime("Kinetra session", 160, 120);
    ScriptedSession direct(SessionRequest::none, 0, true);
    CHECK(run_session(runtime, direct, 2) == 2);
    CHECK(direct.advances == 2);
    CHECK(direct.renders == 2);
    CHECK_THROWS_AS(run_session(runtime, direct, -1), std::invalid_argument);
}

TEST_CASE("run_session ends on any request without rendering that frame") {
    Runtime runtime("Kinetra session", 160, 120);
    for (const auto request : {SessionRequest::menu, SessionRequest::configurations, SessionRequest::exit}) {
        CAPTURE(static_cast<int>(request));
        ScriptedSession session(request, 2, true);
        CHECK(run_session(runtime, session, 10) == 1);
        CHECK(session.advances == 2);
        CHECK(session.renders == 1);
    }
}

TEST_CASE("run_launcher returns to the menu on a menu request") {
    Runtime runtime("Kinetra launcher", 160, 120);
    ScriptedApplication application;
    application.request = SessionRequest::menu;
    application.request_at = 2;
    CHECK(run_launcher(runtime, application.registrations(), LaunchSelection{}, 4) == 4);
    CHECK(application.created == 1);
    CHECK_MESSAGE(application.destroyed == 1, "the session is destroyed when it asks for the menu");
    CHECK_MESSAGE(application.renders == 1, "the menu, not the session, draws the frame of the request");
}

TEST_CASE("run_launcher returns to the configuration page on a configurations request") {
    Runtime runtime("Kinetra launcher", 160, 120);
    ScriptedApplication application;
    application.configurations = {"SMALL", "LARGE"};
    application.request = SessionRequest::configurations;
    application.request_at = 2;
    // The first session draws frame 1 and asks for its page in frame 2, which the menu
    // draws. Enter, pressed then, reaches the menu in frame 3 and starts the configuration
    // the page selected, whose session draws that frame.
    application.on_request = [&] {
        if (application.created == 1)
            press(SDLK_RETURN);
    };
    CHECK(run_launcher(runtime, application.registrations(), LaunchSelection{0, 1}, 3) == 3);
    REQUIRE_MESSAGE(application.created == 2, "Enter did not start a session from the configuration page");
    CHECK_MESSAGE(application.started[1] == 1, "the page did not select the configuration that ran");
    CHECK_MESSAGE(application.renders == 2, "the menu, not the session, draws the frame of the request");
    CHECK(application.destroyed == 2);
}

TEST_CASE("run_launcher titles the window after the menu or the running session from the first frame") {
    Runtime runtime("Kinetra launcher", 160, 120);
    const auto title = [] { return std::string(SDL_GetWindowTitle(SDL_GL_GetCurrentWindow())); };
    ScriptedApplication application;
    CHECK(run_launcher(runtime, application.registrations(), std::nullopt, 1) == 1);
    CHECK(title() == launcher_window_title);
    CHECK(run_launcher(runtime, application.registrations(), LaunchSelection{}, 1) == 1);
    CHECK(title() == "SCRIPTED SESSION");
    application.request = SessionRequest::menu;
    application.request_at = 1;
    CHECK(run_launcher(runtime, application.registrations(), LaunchSelection{}, 1) == 1);
    CHECK_MESSAGE(title() == launcher_window_title, "returning to the menu restores the menu title");
}

TEST_CASE("run_launcher ends on an exit request without rendering that frame") {
    Runtime runtime("Kinetra launcher", 160, 120);
    ScriptedApplication application;
    application.request = SessionRequest::exit;
    application.request_at = 2;
    CHECK(run_launcher(runtime, application.registrations(), LaunchSelection{}, 10) == 1);
    CHECK(application.created == 1);
    CHECK(application.destroyed == 1);
    CHECK(application.renders == 1);
}

TEST_CASE("the launcher menu key is configurable") {
    {
        Runtime runtime("Kinetra launcher", 160, 120);
        ScriptedApplication application;
        press(SDLK_m);
        run_launcher(runtime, application.registrations(), LaunchSelection{}, 2);
        CHECK_MESSAGE(application.renders == 0, "M returns to the menu by default, before the session draws");
    }
    {
        Runtime runtime("Kinetra launcher", 160, 120);
        ScriptedApplication application;
        press(SDLK_m);
        run_launcher(runtime, application.registrations(), LaunchSelection{}, 2,
                     LauncherBindings{std::nullopt, true});
        CHECK_MESSAGE(application.renders == 2, "a disabled menu key leaves the session running");
    }
}

TEST_CASE("escape follows the session during a run and the bindings in the menu") {
    {
        Runtime runtime("Kinetra launcher", 160, 120);
        ScriptedApplication application;
        application.escape_closes = false;
        press(SDLK_ESCAPE);
        CHECK_MESSAGE(run_launcher(runtime, application.registrations(), LaunchSelection{}, 3) == 3,
                      "a session that keeps Escape keeps the window open");
    }
    {
        Runtime runtime("Kinetra launcher", 160, 120);
        ScriptedApplication application;
        press(SDLK_ESCAPE);
        CHECK_MESSAGE(run_launcher(runtime, application.registrations(), LaunchSelection{}, 3) == 0,
                      "a session that gives up Escape closes the window");
    }
    {
        Runtime runtime("Kinetra launcher", 160, 120);
        ScriptedApplication application;
        press(SDLK_ESCAPE);
        CHECK_MESSAGE(run_launcher(runtime, application.registrations(), std::nullopt, 3,
                                   LauncherBindings{Key::m, false}) == 3,
                      "the menu keeps the window open when Escape is not bound");
    }
    {
        Runtime runtime("Kinetra launcher", 160, 120);
        ScriptedApplication application;
        press(SDLK_ESCAPE);
        CHECK_MESSAGE(run_launcher(runtime, application.registrations(), std::nullopt, 3) == 0,
                      "Escape closes the menu by default");
    }
}
