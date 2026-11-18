// Graphics tests for the SDL2/OpenGL runtime and the session and launcher loops. They
// open real windows, so they run only with KINETRA_TEST_GRAPHICS. Only one Runtime may
// exist at a time; each case owns its own and closes it before the next case starts.
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <kinetra/debug_view.h>
#include <kinetra/engine.h>
#include <kinetra/launcher.h>
#include <kinetra/session.h>

#include <doctest/doctest.h>

#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
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
    event = {};
    event.type = SDL_MOUSEWHEEL;
    event.wheel.y = 2;
    send(event);
    event = {};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    send(event);
    runtime.poll();
    REQUIRE_MESSAGE((runtime.input().clicked && runtime.input().wheel == 2), "Mouse input lost");
    runtime.poll();
    REQUIRE_MESSAGE((!runtime.input().clicked && runtime.input().mouse_down && runtime.input().wheel == 0),
                    "Held shooting input or mouse transitions lost");
    event = {};
    event.type = SDL_MOUSEBUTTONUP;
    event.button.button = SDL_BUTTON_LEFT;
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
