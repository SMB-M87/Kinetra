// Graphics tests for the SDL2/OpenGL runtime and the session and launcher loops. They
// open real windows, so they run only with KINETRA_TEST_GRAPHICS. Only one Runtime may
// exist at a time; each case owns its own and closes it before the next case starts.
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <kinetra/runtime/debug_view.h>
#include <kinetra/runtime/engine.h>
#include <kinetra/runtime/launcher.h>
#include <kinetra/runtime/session.h>
#include <kinetra/runtime/settings_view.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
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
    int created{}, destroyed{}, renders{};
    std::vector<ApplicationRegistration> registrations() {
        return {{{"SCRIPTED", "TEST", {}, 0}, [this](Camera2D &, std::size_t) -> std::unique_ptr<Session> {
                     ++created;
                     return std::make_unique<ScriptedSession>(request, request_at, escape_closes, &destroyed,
                                                              &renders);
                 }}};
    }
};
} // namespace

TEST_CASE("input drawing and resizing reach SDL and OpenGL") {
    Runtime runtime("Kinetra graphics test", 320, 240);
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
    // Fill the mouse events as SDL fills real ones: sdl2-compat converts a pushed SDL2
    // event to SDL3 from preciseY and state, so a wheel event with only `y` arrives as 0.
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
    glFinish();
    // Software GLX can retain the rendered image in either swap buffer;
    // both are valid for verifying that the primitive batch reached GL.
    unsigned char pixels[4]{};
    glReadBuffer(GL_FRONT);
    glReadPixels(160, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (pixels[1] < 200) {
        glReadBuffer(GL_BACK);
        glReadPixels(160, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }
    REQUIRE_MESSAGE((pixels[1] > 200 && pixels[0] < 20 && pixels[2] < 20), "Rendered center pixel missing");
    REQUIRE_MESSAGE(glGetError() == GL_NO_ERROR, "OpenGL reported an error");
    runtime.begin({0, 0, 0, 1});
    const Vec2 polygon[]{{-2, -2}, {2, -2}, {0, 2}};
    runtime.polygon(polygon, {0, 1, 0});
    runtime.end();
    glFinish();
    glReadBuffer(GL_FRONT);
    glReadPixels(160, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (pixels[1] < 200) {
        glReadBuffer(GL_BACK);
        glReadPixels(160, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }
    REQUIRE_MESSAGE((pixels[1] > 200 && pixels[0] < 20), "Filled polygon missing from rendered batch");
    for (const float zoom : {2.5f, 50.f}) {
        runtime.camera.pixels_per_unit = zoom;
        runtime.begin({0, 0, 0, 1});
        draw_debug_vector(runtime, {}, {0.0001f, 0}, {0, 1, 0});
        runtime.end();
        glFinish();
        glReadBuffer(GL_FRONT);
        glReadPixels(168, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        if (pixels[1] < 200) {
            glReadBuffer(GL_BACK);
            glReadPixels(168, 120, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        }
        REQUIRE_MESSAGE(pixels[1] > 200, "Small debug vector vanished at camera zoom");
    }
    // Read a full circle back to verify both coverage and a fractional-alpha edge.
    runtime.camera.pixels_per_unit = 50;
    runtime.begin({0, 0, 0, 1});
    runtime.circle({}, 1.73f, {0, 1, 0, 1});
    runtime.end();
    glFinish();
    std::vector<unsigned char> image(320 * 240 * 4);
    auto read_circle = [&] { glReadPixels(0, 0, 320, 240, GL_RGBA, GL_UNSIGNED_BYTE, image.data()); };
    glReadBuffer(GL_FRONT);
    read_circle();
    if (image[(120 * 320 + 160) * 4 + 1] < 200) {
        glReadBuffer(GL_BACK);
        read_circle();
    }
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
        REQUIRE_MESSAGE((runtime.camera.viewport.x == size.x && runtime.camera.viewport.y == size.y),
                        "Camera did not follow window resize");
        const Vec2 center = runtime.camera.world_to_screen(runtime.camera.position);
        REQUIRE_MESSAGE((center.x == size.x / 2 && center.y == size.y / 2),
                        "Camera center moved after resize");
        const Vec2 picked = runtime.camera.screen_to_world(center + Vec2{20, -20});
        REQUIRE_MESSAGE(
            (picked.x == runtime.camera.position.x + 1 && picked.y == runtime.camera.position.y + 1),
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
        glFinish();
        glReadBuffer(GL_FRONT);
        glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        if (pixels[1] < 200) {
            glReadBuffer(GL_BACK);
            glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        }
        REQUIRE_MESSAGE((pixels[1] > 200 && pixels[0] < 20 && pixels[2] < 20),
                        "Rendered center pixel missing after resize");
        REQUIRE_MESSAGE(glGetError() == GL_NO_ERROR, "OpenGL error after resize");
    }
    event = {};
    event.type = SDL_QUIT;
    send(event);
    REQUIRE_MESSAGE(!runtime.poll(), "Window quit ignored");
}

TEST_CASE("presented frames are timed and the frame-time graph draws them") {
    Runtime runtime("Kinetra frame times", 320, 200);
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
    glFinish();
    // Read the bar area's bottom row, just above the bars' base, from the presented frame.
    const Vec2 origin = FrameTimeGraph::origin(runtime.camera.viewport);
    const int row = int(runtime.camera.viewport.y - (origin.y + FrameTimeGraph::height - 2));
    // The front buffer is undefined after a swap on some drivers; count the better buffer.
    int green = 0;
    for (const auto buffer : {GL_FRONT, GL_BACK}) {
        std::vector<unsigned char> pixels(240 * 3);
        glReadBuffer(buffer);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(int(origin.x), row, 240, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        int count = 0;
        for (std::size_t i = 0; i < pixels.size(); i += 3)
            if (pixels[i] < 120 && pixels[i + 1] > 180 && pixels[i + 2] < 140)
                ++count;
        green = std::max(green, count);
    }
    CHECK_MESSAGE(green > 200, "Short frames were not drawn as green bars");
    CHECK(glGetError() == GL_NO_ERROR);
}

namespace {
/// Whether the presented frame shows `expected` at logical screen pixel `point` (Y down),
/// within rounding. The front buffer is undefined after a swap on some drivers, so either
/// buffer may hold the frame.
bool presented(const Runtime &runtime, Vec2 point, Color expected) {
    for (const auto buffer : {GL_FRONT, GL_BACK}) {
        unsigned char pixel[4]{};
        glReadBuffer(buffer);
        glReadPixels(int(point.x), int(runtime.camera.viewport.y - 1 - point.y), 1, 1, GL_RGBA,
                     GL_UNSIGNED_BYTE, pixel);
        const auto near = [](unsigned char channel, float value) {
            return std::abs(float(channel) - value * 255) <= 3;
        };
        if (near(pixel[0], expected.r) && near(pixel[1], expected.g) && near(pixel[2], expected.b))
            return true;
    }
    return false;
}
} // namespace

TEST_CASE("the debug menu draws its panel, section band, marks and swatch") {
    Runtime runtime("Kinetra debug menu", 640, 480);
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
    glFinish();
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
    glFinish();
    CHECK_MESSAGE(presented(runtime, at(120, DebugMenu::row_top(3) + 8), overlay::band_color), "hovered row");
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("the settings panel and its toolbar draw in the shared style") {
    Runtime runtime("Kinetra settings", 640, 480);
    REQUIRE(runtime.poll());
    bool statistics = true, debug = true;
    SettingsOverlay overlay({ToggleRow{"STATISTICS", &statistics}, ToggleRow{"DEBUG MODE", &debug},
                             HelpSection{"CONTROLS", {"WASD  MOVE"}}},
                            &debug);
    runtime.begin({0, 0, 0, 1});
    overlay.draw(runtime);
    runtime.end();
    glFinish();
    CHECK_MESSAGE(presented(runtime, {6, 6}, overlay::button_color), "SETTINGS toolbar button");
    CHECK_MESSAGE(presented(runtime, {70, 6}, overlay::button_color), "DEBUG toolbar button in debug mode");
    Input open;
    open.pressed[std::size_t(Key::control)] = true;
    open.mouse = {630, 470};
    REQUIRE(overlay.update(open, runtime.camera));
    runtime.begin({0, 0, 0, 1});
    overlay.draw(runtime);
    runtime.end();
    glFinish();
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

TEST_CASE("run_session ends on a menu or exit request without rendering that frame") {
    Runtime runtime("Kinetra session", 160, 120);
    for (const auto request : {SessionRequest::menu, SessionRequest::exit}) {
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
