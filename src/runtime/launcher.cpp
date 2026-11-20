#include <chrono>
#include <kinetra/runtime/bitmap_text.h>
#include <kinetra/runtime/launcher.h>
#include <stdexcept>

namespace kinetra {
MenuLayout::MenuLayout(Vec2 viewport, std::size_t rows)
    : scale(std::min({2.f, viewport.x / 320,
                      viewport.y / (150.f + static_cast<float>(std::min(rows, page_size) * 40))})),
      visible_rows(std::min(rows, page_size)) {
    if (!finite(viewport) || viewport.x <= 0 || viewport.y <= 0)
        throw std::invalid_argument("Invalid menu viewport");
    origin = (viewport - Vec2{300, 130.f + static_cast<float>(visible_rows * 40)} * scale) / 2;
}
Vec2 MenuLayout::footer_center(float x) const {
    return origin + Vec2{x, 110.f + static_cast<float>(visible_rows * 40)} * scale;
}
bool MenuLayout::footer_contains(Vec2 point, float x) const {
    const auto offset = point - footer_center(x);
    return std::abs(offset.x) <= 40 * scale && std::abs(offset.y) <= 14 * scale;
}
Vec2 MenuLayout::row_center(std::size_t index) const {
    return origin + Vec2{150, 80.f + static_cast<float>(index * 40)} * scale;
}
bool MenuLayout::contains(std::size_t index, Vec2 point) const {
    const auto offset = point - row_center(index);
    return std::abs(offset.x) <= 140 * scale && std::abs(offset.y) <= 16 * scale;
}
LaunchMenu::LaunchMenu(std::vector<LaunchChoice> choices, std::string heading)
    : choices_(std::move(choices)), heading_(std::move(heading)) {
    if (choices_.empty())
        throw std::invalid_argument("Launcher requires at least one application");
    for (const auto &choice : choices_) {
        if (choice.name.empty() ||
            (choice.configurations.empty() ? choice.default_configuration != 0
                                           : choice.default_configuration >= choice.configurations.size()))
            throw std::invalid_argument("Invalid launcher name or default configuration");
        configurations_.push_back(choice.default_configuration);
    }
}
std::size_t LaunchMenu::selected() const {
    return configuring_ ? configuration() : application_;
}
std::size_t LaunchMenu::row_count() const {
    return configuring_ ? choices_[application_].configurations.size() : choices_.size();
}
std::string_view LaunchMenu::row_name(std::size_t row) const {
    return configuring_ ? choices_[application_].configurations.at(row) : choices_.at(row).name;
}
std::string_view LaunchMenu::description() const {
    return choices_[application_].description;
}
std::optional<LaunchSelection> LaunchMenu::update(const Input &input, Vec2 viewport) {
    const bool pointer_moved = input.mouse.x != pointer_.x || input.mouse.y != pointer_.y;
    pointer_ = input.mouse;
    if (configuring_ && input.hit(Key::m)) {
        configuring_ = false;
        return std::nullopt;
    }
    auto &selection = configuring_ ? configurations_[application_] : application_;
    if (input.hit(Key::down))
        selection = (selection + 1) % row_count();
    if (input.hit(Key::up))
        selection = (selection + row_count() - 1) % row_count();
    // A resting pointer must not undo keyboard navigation on the next frame.
    if (pointer_moved && !input.hit(Key::down) && !input.hit(Key::up)) {
        const MenuLayout layout(viewport, row_count());
        const auto first = selection / MenuLayout::page_size * MenuLayout::page_size;
        for (std::size_t i = 0; i < layout.visible_rows && first + i < row_count(); ++i)
            if (layout.contains(i, input.mouse)) {
                selection = first + i;
                break;
            }
    }
    bool confirm = input.hit(Key::enter) || input.hit(Key::space);
    if (input.clicked) {
        const MenuLayout layout(viewport, row_count());
        const auto first = selection / MenuLayout::page_size * MenuLayout::page_size;
        if (layout.footer_contains(input.mouse, 150)) {
            exit_requested_ = true;
            return std::nullopt;
        }
        if (row_count() > MenuLayout::page_size) {
            if (layout.footer_contains(input.mouse, 50)) {
                selection = first == 0 ? (row_count() - 1) / MenuLayout::page_size * MenuLayout::page_size
                                       : first - MenuLayout::page_size;
                return std::nullopt;
            }
            if (layout.footer_contains(input.mouse, 250)) {
                selection = first + MenuLayout::page_size < row_count() ? first + MenuLayout::page_size : 0;
                return std::nullopt;
            }
        }
        for (std::size_t i = 0; i < layout.visible_rows && first + i < row_count(); ++i)
            if (layout.contains(i, input.mouse)) {
                selection = first + i;
                confirm = true;
                break;
            }
    }
    if (!confirm)
        return std::nullopt;
    if (!configuring_ && !choices_[application_].configurations.empty()) {
        configuring_ = true;
        return std::nullopt;
    }
    configuring_ = false;
    return LaunchSelection{application_, configuration()};
}
void LaunchMenu::draw(Runtime &runtime, std::string_view status) const {
    const MenuLayout layout(runtime.camera.viewport, row_count());
    runtime.begin();
    draw_text(runtime, heading_, layout.origin + Vec2{10, 10} * layout.scale, layout.scale * 3,
              {0.55f, 1, 0.35f});
    if (configuring_)
        draw_text(runtime, "SELECT CONFIGURATION", layout.origin + Vec2{10, 44} * layout.scale, layout.scale,
                  {0.8f, 0.85f, 1});
    const auto first = selected() / MenuLayout::page_size * MenuLayout::page_size;
    for (std::size_t i = 0; i < layout.visible_rows && first + i < row_count(); ++i) {
        runtime.box(runtime.camera.screen_to_world(layout.row_center(i)),
                    Vec2{140, 16} * (layout.scale / runtime.camera.pixels_per_unit), 0,
                    first + i == selected() ? Color{0.15f, 0.3f, 0.4f} : Color{0.06f, 0.1f, 0.16f});
        draw_text(runtime, row_name(first + i), layout.row_center(i) - Vec2{130, 4} * layout.scale,
                  layout.scale, {0.95f, 0.95f, 1});
    }
    auto button = [&](std::string_view label, float x) {
        draw_text(runtime, label,
                  layout.footer_center(x) -
                      Vec2{(static_cast<float>(label.size()) * 6.f - 1) / 2, 4} * layout.scale,
                  layout.scale,
                  layout.footer_contains(pointer_, x) ? Color{0.25f, 0.65f, 1.f} : Color{0.8f, 0.85f, 1});
    };
    button("EXIT", 150);
    if (row_count() > MenuLayout::page_size) {
        button("PREVIOUS", 50);
        button("NEXT", 250);
    }
    if (!status.empty())
        draw_text(runtime, status.substr(0, 80), {8, runtime.camera.viewport.y - 16},
                  std::min(1.f, runtime.camera.viewport.x / 490), {1, 0.5f, 0.35f});
    runtime.end();
}
SessionHost::SessionHost(std::vector<ApplicationRegistration> applications)
    : applications_(std::move(applications)) {
    LaunchMenu validate(choices());
    for (const auto &application : applications_)
        if (!application.create)
            throw std::invalid_argument("Missing application factory");
}
std::vector<LaunchChoice> SessionHost::choices() const {
    std::vector<LaunchChoice> result;
    for (const auto &application : applications_)
        result.push_back(application.choice);
    return result;
}
void SessionHost::start(LaunchSelection selection, Camera2D &camera) {
    const auto &entry = applications_.at(selection.application);
    const auto count = entry.choice.configurations.size();
    if ((count == 0 && selection.configuration != 0) || (count && selection.configuration >= count))
        throw std::out_of_range("Unknown application configuration");
    auto next_camera = camera;
    auto next = entry.create(next_camera, selection.configuration);
    if (!next)
        throw std::runtime_error("Application factory returned no session");
    session_ = std::move(next);
    camera = next_camera;
    fresh_ = true;
}
void SessionHost::return_to_menu() {
    session_.reset();
    fresh_ = false;
}
void SessionHost::advance(double elapsed, const Input &input, Camera2D &camera) {
    if (!session_)
        return;
    if (fresh_) {
        session_->advance(0, {}, camera);
        fresh_ = false;
    } else
        session_->advance(elapsed, input, camera);
}
int run_launcher(Runtime &runtime, std::vector<ApplicationRegistration> applications,
                 std::optional<LaunchSelection> initial, int frame_limit, LauncherBindings bindings) {
    if (frame_limit < 0)
        throw std::invalid_argument("Negative frame limit");
    SessionHost host(std::move(applications));
    LaunchMenu menu(host.choices());
    if (initial)
        host.start(*initial, runtime.camera);
    using Clock = std::chrono::steady_clock;
    // Titles change only on transitions; the periodic refresh covers a session whose title
    // changes on its own without asking the window manager every frame.
    constexpr auto title_refresh = std::chrono::milliseconds(500);
    auto previous = Clock::now();
    std::optional<Clock::time_point> last_title; // empty until the first frame sets it
    int frames = 0;
    while (runtime.poll(host.session() ? host.session()->close_on_escape() : bindings.escape_closes_menu)) {
        const auto now = Clock::now();
        const double elapsed = std::chrono::duration<double>(now - previous).count();
        previous = now;
        bool switched = false;
        if (host.session() && bindings.menu_key && runtime.input().hit(*bindings.menu_key)) {
            host.return_to_menu();
            switched = true;
        } else if (!host.session()) {
            if (const auto selection = menu.update(runtime.input(), runtime.camera.viewport)) {
                host.start(*selection, runtime.camera);
                switched = true;
            }
        }
        if (menu.exit_requested())
            break;
        // World construction must not accrue catch-up simulation time.
        if (switched)
            previous = Clock::now();
        if (host.session()) {
            host.advance(elapsed, runtime.input(), runtime.camera);
            const SessionRequest request = host.session()->request();
            if (request == SessionRequest::exit)
                break;
            if (request == SessionRequest::menu) {
                // Destroy before the menu draws, so a session never coexists with the next one.
                host.return_to_menu();
                switched = true;
            }
        }
        if (host.session())
            host.session()->render(runtime);
        else
            menu.draw(runtime);
        if (switched || !last_title || now - *last_title > title_refresh) {
            runtime.title(host.session() ? host.session()->title() : std::string(launcher_window_title));
            last_title = now;
        }
        if (++frames == frame_limit)
            break;
    }
    return frames;
}
} // namespace kinetra
