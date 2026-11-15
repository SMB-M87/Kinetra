#include <SDL2/SDL.h>
#include <cmath>
#include <glad/glad.h>
#include <iostream>
#include <kinetra/debug_view.h>
#include <kinetra/engine.h>
#include <kinetra/session.h>
#include <stdexcept>
#include <vector>
using namespace kinetra;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void send(SDL_Event event) {
    if (SDL_PushEvent(&event) < 0)
        throw std::runtime_error(SDL_GetError());
}
} // namespace
int main() {
    try {
        {
            Runtime runtime("Kinetra graphics test", 320, 240);
            runtime.poll();
            SDL_Event event{};
            event.type = SDL_KEYDOWN;
            event.key.keysym.sym = SDLK_w;
            send(event);
            runtime.poll();
            require(runtime.input().down(Key::w) && runtime.input().hit(Key::w), "Key press lost");
            runtime.poll();
            require(runtime.input().down(Key::w) && !runtime.input().hit(Key::w), "Key transition repeated");
            event.type = SDL_KEYUP;
            send(event);
            runtime.poll();
            require(!runtime.input().down(Key::w), "Key release lost");
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
                require(runtime.input().down(key) && runtime.input().hit(key), "Sandbox key press lost");
                event.type = SDL_KEYUP;
                send(event);
                runtime.poll();
                require(!runtime.input().down(key), "Sandbox key release lost");
            }
            event.type = SDL_KEYDOWN;
            event.key.keysym.sym = SDLK_a;
            send(event);
            event = {};
            event.type = SDL_WINDOWEVENT;
            event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
            send(event);
            runtime.poll();
            require(!runtime.input().down(Key::a), "Focus loss retained key");
            event = {};
            event.type = SDL_MOUSEWHEEL;
            event.wheel.y = 2;
            send(event);
            event = {};
            event.type = SDL_MOUSEBUTTONDOWN;
            event.button.button = SDL_BUTTON_LEFT;
            send(event);
            runtime.poll();
            require(runtime.input().clicked && runtime.input().wheel == 2, "Mouse input lost");
            runtime.poll();
            require(!runtime.input().clicked && runtime.input().mouse_down && runtime.input().wheel == 0,
                    "Held shooting input or mouse transitions lost");
            event = {};
            event.type = SDL_MOUSEBUTTONUP;
            event.button.button = SDL_BUTTON_LEFT;
            send(event);
            runtime.poll();
            require(!runtime.input().mouse_down, "Mouse release retained shooting");
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
            require(pixels[1] > 200 && pixels[0] < 20 && pixels[2] < 20, "Rendered center pixel missing");
            require(glGetError() == GL_NO_ERROR, "OpenGL reported an error");
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
                require(pixels[1] > 200, "Small debug vector vanished at camera zoom");
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
                    const float dx = x + 0.5f - 160, dy = y + 0.5f - 120;
                    const float distance = std::sqrt(dx * dx + dy * dy);
                    const auto green = image[(y * 320 + x) * 4 + 1];
                    if (distance < 85.5f)
                        require(green > 240, "Circle polygon left an interior gap");
                    if (green > 10 && green < 240)
                        ++softened;
                }
            require(softened > 100, "Circle edge has no antialiasing coverage");
            runtime.camera.pixels_per_unit = 20;
            SDL_Window *window = SDL_GL_GetCurrentWindow();
            require(window && (SDL_GetWindowFlags(window) & SDL_WINDOW_RESIZABLE),
                    "Window must support resizing");
            // Resize the real window in both directions; synthetic events alone
            // would not exercise the logical size and drawable size queries.
            for (const auto size : {Vec2{640, 360}, Vec2{200, 150}}) {
                SDL_SetWindowSize(window, int(size.x), int(size.y));
                runtime.poll();
                require(runtime.camera.viewport.x == size.x && runtime.camera.viewport.y == size.y,
                        "Camera did not follow window resize");
                const Vec2 center = runtime.camera.world_to_screen(runtime.camera.position);
                require(center.x == size.x / 2 && center.y == size.y / 2, "Camera center moved after resize");
                const Vec2 picked = runtime.camera.screen_to_world(center + Vec2{20, -20});
                require(picked.x == runtime.camera.position.x + 1 &&
                            picked.y == runtime.camera.position.y + 1,
                        "Picking scale changed after resize");
                runtime.begin({0, 0, 0, 1});
                runtime.box(runtime.camera.position, {2, 2}, 0, {0, 1, 0, 1});
                runtime.end();
                int width, height;
                SDL_GL_GetDrawableSize(window, &width, &height);
                GLint viewport[4]{};
                glGetIntegerv(GL_VIEWPORT, viewport);
                require(viewport[2] == width && viewport[3] == height,
                        "OpenGL viewport did not follow drawable resize");
                glFinish();
                glReadBuffer(GL_FRONT);
                glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                if (pixels[1] < 200) {
                    glReadBuffer(GL_BACK);
                    glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                }
                require(pixels[1] > 200 && pixels[0] < 20 && pixels[2] < 20,
                        "Rendered center pixel missing after resize");
                require(glGetError() == GL_NO_ERROR, "OpenGL error after resize");
            }
            event = {};
            event.type = SDL_QUIT;
            send(event);
            require(!runtime.poll(), "Window quit ignored");
        }
        Runtime reopened("Kinetra reopened", 160, 120);
        require(reopened.poll(), "Runtime could not reopen after destruction");
        struct DirectSession final : Session {
            int updates{}, draws{};
            void advance(double elapsed, const Input &, Camera2D &) override {
                require(elapsed >= 0, "Negative frame time");
                ++updates;
            }
            void render(Runtime &runtime) override {
                ++draws;
                runtime.begin();
                runtime.end();
            }
            std::string title() override { return "DIRECT SESSION"; }
        } direct;
        require(run_session(reopened, direct, 2) == 2 && direct.updates == 2 && direct.draws == 2,
                "Direct session runner did not honor frame limit");
        bool invalid_limit = false;
        try {
            run_session(reopened, direct, -1);
        } catch (const std::invalid_argument &) {
            invalid_limit = true;
        }
        require(invalid_limit, "Direct session runner accepted negative limit");
        std::cout << "PASS engine graphics, resize, input, focus, quit and lifecycle\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
