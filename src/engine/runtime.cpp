#include <SDL2/SDL.h>
#include <chrono>
#include <cmath>
#include <glad/glad.h>
#include <kinetra/engine.h>
#include <optional>
#include <stdexcept>
#include <vector>

namespace kinetra {
namespace {

bool runtime_active = false;

std::optional<Key> key(SDL_Keycode code) {
    switch (code) {
    case SDLK_w:
        return Key::w;
    case SDLK_a:
        return Key::a;
    case SDLK_s:
        return Key::s;
    case SDLK_d:
        return Key::d;
    case SDLK_q:
        return Key::q;
    case SDLK_e:
        return Key::e;
    case SDLK_UP:
        return Key::up;
    case SDLK_DOWN:
        return Key::down;
    case SDLK_LEFT:
        return Key::left;
    case SDLK_RIGHT:
        return Key::right;
    case SDLK_SPACE:
        return Key::space;
    case SDLK_r:
        return Key::r;
    case SDLK_g:
        return Key::g;
    case SDLK_TAB:
        return Key::tab;
    case SDLK_f:
        return Key::f;
    case SDLK_p:
        return Key::p;
    case SDLK_t:
        return Key::t;
    case SDLK_c:
        return Key::c;
    case SDLK_h:
        return Key::h;
    case SDLK_RETURN:
        return Key::enter;
    case SDLK_m:
        return Key::m;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        return Key::shift;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        return Key::control;
    case SDLK_v:
        return Key::v;
    case SDLK_BACKQUOTE:
        return Key::backquote;
    case SDLK_1:
        return Key::one;
    case SDLK_2:
        return Key::two;
    case SDLK_3:
        return Key::three;
    case SDLK_4:
        return Key::four;
    case SDLK_5:
        return Key::five;
    case SDLK_6:
        return Key::six;
    case SDLK_ESCAPE:
        return Key::escape;
    default:
        return {};
    }
}

GLuint shader(GLenum type, const char *source) {
    const GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);
    GLint success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char message[2048]{};
        glGetShaderInfoLog(id, sizeof(message), nullptr, message);
        glDeleteShader(id);
        throw std::runtime_error(std::string("Shader compilation failed: ") + message);
    }
    return id;
}

} // namespace

struct Runtime::Impl {
    SDL_Window *window{};
    SDL_GLContext context{};
    GLuint program{}, vao{}, vbo{};
    Input input;
    std::string title;
    float drawable_scale{1};
    bool running{true}, owns_sdl{}, gl_ready{};
    struct Vertex {
        float x, y, r, g, b, a;
    };
    std::vector<Vertex> vertices;
    FrameTimes frame_times;
    /// When the previous frame was presented; empty before the first one.
    std::optional<std::chrono::steady_clock::time_point> presented;

    ~Impl() {
        if (gl_ready) {
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
            glDeleteProgram(program);
        }
        if (context)
            SDL_GL_DeleteContext(context);
        if (window)
            SDL_DestroyWindow(window);
        if (owns_sdl) {
            SDL_Quit();
            runtime_active = false;
        }
    }
};

Runtime::Runtime(const std::string &text, int width, int height) : impl_(std::make_unique<Impl>()) {
    if (runtime_active)
        throw std::runtime_error("Only one Kinetra Runtime may be active");
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Window dimensions must be positive");
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(SDL_GetError());
    impl_->owns_sdl = true;
    runtime_active = true;
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0)
        throw std::runtime_error(SDL_GetError());
    impl_->window =
        SDL_CreateWindow(text.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!impl_->window)
        throw std::runtime_error(SDL_GetError());
    impl_->context = SDL_GL_CreateContext(impl_->window);
    if (!impl_->context)
        throw std::runtime_error(SDL_GetError());
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
        throw std::runtime_error("OpenGL 3.3 functions could not be loaded");
    impl_->gl_ready = true;
    // Adaptive synchronization avoids halving the presentation rate on a missed refresh.
    if (SDL_GL_SetSwapInterval(-1) != 0 && SDL_GL_SetSwapInterval(1) != 0)
        SDL_Log("VSync unavailable: %s", SDL_GetError());
    const GLuint vs = shader(GL_VERTEX_SHADER, R"(#version 330 core
layout(location=0) in vec2 point;
layout(location=1) in vec4 tint;
out vec4 color;
void main() { gl_Position=vec4(point,0,1); color=tint; }
)");
    GLuint fs{};
    try {
        fs = shader(GL_FRAGMENT_SHADER, R"(#version 330 core
in vec4 color;
out vec4 pixel;
void main() { pixel=color; }
)");
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }
    impl_->program = glCreateProgram();
    glAttachShader(impl_->program, vs);
    glAttachShader(impl_->program, fs);
    glLinkProgram(impl_->program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked;
    glGetProgramiv(impl_->program, GL_LINK_STATUS, &linked);
    if (!linked) {
        char message[2048]{};
        glGetProgramInfoLog(impl_->program, sizeof(message), nullptr, message);
        throw std::runtime_error(std::string("Shader link failed: ") + message);
    }
    glGenVertexArrays(1, &impl_->vao);
    glGenBuffers(1, &impl_->vbo);
    glBindVertexArray(impl_->vao);
    glBindBuffer(GL_ARRAY_BUFFER, impl_->vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Impl::Vertex), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Impl::Vertex),
                          reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    impl_->title = text;
    camera.viewport = {float(width), float(height)};
}

Runtime::~Runtime() = default;
const Input &Runtime::input() const {
    return impl_->input;
}
void Runtime::title(const std::string &text) {
    if (impl_->title != text) {
        SDL_SetWindowTitle(impl_->window, text.c_str());
        impl_->title = text;
    }
}

bool Runtime::poll(bool close_on_escape) {
    auto &in = impl_->input;
    in.pressed.fill(false);
    in.wheel = 0;
    in.clicked = false;
    in.right_clicked = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
            impl_->running = false;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
            in.held.fill(false);
            in.mouse_down = false;
            in.right_mouse_down = false;
        }
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            if (const auto k = key(event.key.keysym.sym)) {
                const auto index = static_cast<std::size_t>(*k);
                in.held[index] = event.type == SDL_KEYDOWN;
                if (event.type == SDL_KEYDOWN && !event.key.repeat)
                    in.pressed[index] = true;
            }
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            in.clicked = true;
            in.mouse_down = true;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
            in.right_clicked = true;
            in.right_mouse_down = true;
        }
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT)
            in.right_mouse_down = false;
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
            in.mouse_down = false;
        if (event.type == SDL_MOUSEWHEEL)
            in.wheel += float(event.wheel.y) * (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.f : 1.f);
    }
    int x, y, w, h;
    SDL_GetMouseState(&x, &y);
    SDL_GetWindowSize(impl_->window, &w, &h);
    in.mouse = {float(x), float(y)};
    camera.viewport = {float(std::max(w, 1)), float(std::max(h, 1))};
    return impl_->running && !(close_on_escape && in.hit(Key::escape));
}

void Runtime::begin(Color background) {
    camera.validate();
    int w, h;
    SDL_GL_GetDrawableSize(impl_->window, &w, &h);
    impl_->drawable_scale =
        std::min(float(std::max(w, 1)) / camera.viewport.x, float(std::max(h, 1)) / camera.viewport.y);
    glViewport(0, 0, w, h);
    glClearColor(background.r, background.g, background.b, background.a);
    glClear(GL_COLOR_BUFFER_BIT);
    impl_->vertices.clear();
}

void Runtime::triangle(Vec2 a, Vec2 b, Vec2 c, Color color) {
    triangle(a, b, c, color, color, color);
}

void Runtime::triangle(Vec2 a, Vec2 b, Vec2 c, Color ca, Color cb, Color cc) {
    const Vec2 points[]{a, b, c};
    const Color colors[]{ca, cb, cc};
    for (int i = 0; i < 3; ++i) {
        const Vec2 screen = camera.world_to_screen(points[i]);
        const auto color = colors[i];
        impl_->vertices.push_back({screen.x * 2 / camera.viewport.x - 1, 1 - screen.y * 2 / camera.viewport.y,
                                   color.r, color.g, color.b, color.a});
    }
}

void Runtime::circle(Vec2 center, float radius, Color color) {
    const float scale = camera.pixels_per_unit * impl_->drawable_scale;
    const float pixel_radius = radius * scale;
    // Bound the polygon's radial error to roughly a quarter drawable pixel.
    const int segments = int(
        std::clamp(std::ceil(3.14159265359f * std::sqrt(std::max(pixel_radius, 1.f) * 2.f)), 24.f, 256.f));
    const float inner = std::max(0.f, radius - 0.5f / scale);
    const float outer = radius + 0.5f / scale;
    Color transparent = color;
    transparent.a = 0;
    Vec2 previous{1, 0};
    for (int i = 1; i <= segments; ++i) {
        const Vec2 next =
            i == segments
                ? Vec2{1, 0}
                : rotated({1, 0}, static_cast<float>(i) * 6.28318530718f / static_cast<float>(segments));
        const Vec2 a = center + previous * inner, b = center + next * inner;
        const Vec2 c = center + previous * outer, d = center + next * outer;
        triangle(center, a, b, color);
        triangle(a, c, d, color, transparent, transparent);
        triangle(a, d, b, color, transparent, color);
        previous = next;
    }
}

void Runtime::polygon(std::span<const Vec2> vertices, Color color) {
    if (vertices.size() < 3)
        throw std::invalid_argument("Polygon drawing requires at least three vertices");
    for (auto vertex : vertices)
        if (!finite(vertex))
            throw std::invalid_argument("Polygon drawing requires finite vertices");
    for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
        triangle(vertices[0], vertices[i], vertices[i + 1], color);
}

void Runtime::box(Vec2 center, Vec2 extent, float angle, Color color) {
    const Vec2 a = center + rotated({-extent.x, -extent.y}, angle);
    const Vec2 b = center + rotated({extent.x, -extent.y}, angle);
    const Vec2 c = center + rotated(extent, angle);
    const Vec2 d = center + rotated({-extent.x, extent.y}, angle);
    triangle(a, b, c, color);
    triangle(a, c, d, color);
}

void Runtime::line(Vec2 from, Vec2 to, float width, Color color) {
    const Vec2 side = perpendicular(normalized(to - from)) * (width / 2);
    triangle(from - side, to - side, to + side, color);
    triangle(from - side, to + side, from + side, color);
}

void Runtime::end() {
    glUseProgram(impl_->program);
    glBindVertexArray(impl_->vao);
    glBindBuffer(GL_ARRAY_BUFFER, impl_->vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(impl_->vertices.size() * sizeof(Impl::Vertex)),
                 impl_->vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(impl_->vertices.size()));
    SDL_GL_SwapWindow(impl_->window);
    // Measured after the swap returns, so a blocking VSync or a stalled compositor counts.
    const auto now = std::chrono::steady_clock::now();
    if (impl_->presented)
        impl_->frame_times.record(std::chrono::duration<double>(now - *impl_->presented).count());
    impl_->presented = now;
}

const FrameTimes &Runtime::frame_times() const {
    return impl_->frame_times;
}

} // namespace kinetra
