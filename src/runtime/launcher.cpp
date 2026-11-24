#include <chrono>
#include <kinetra/runtime/bitmap_text.h>
#include <kinetra/runtime/launcher.h>
#include <stdexcept>

namespace kinetra {
namespace {
constexpr Vec2 preview_size{270, 214};
constexpr Vec2 preview_inset{10, 10};
constexpr Vec2 image_slot{250, 140};
constexpr float panel_gap = 18;

void draw_preview(Runtime &runtime, const LaunchChoice &choice, Vec2 origin, float scale) {
    const auto world = [&](Vec2 point) { return runtime.camera.screen_to_world(origin + point * scale); };
    const float units = scale / runtime.camera.pixels_per_unit;
    runtime.box(world(preview_size / 2), preview_size * (units / 2), 0, {0.06f, 0.1f, 0.16f});
    Vec2 description_origin = preview_inset;
    if (choice.preview) {
        const Vec2 pixels{static_cast<float>(choice.preview->width()),
                          static_cast<float>(choice.preview->height())};
        const Vec2 size = pixels * std::min(image_slot.x / pixels.x, image_slot.y / pixels.y);
        runtime.image(choice.preview, world(preview_inset + (image_slot - size) / 2), size * units);
        description_origin.y += image_slot.y + preview_inset.y;
    }
    // The bitmap font advances six units per character. Keep the panel size fixed as
    // choices change, preferring word boundaries and honoring explicit line breaks.
    constexpr std::size_t columns = static_cast<std::size_t>(image_slot.x / 6);
    std::string_view text = choice.description;
    for (int row = 0; row < 4 && !text.empty(); ++row) {
        const auto line_end = std::min(text.find('\n'), text.size());
        auto length = std::min(line_end, columns);
        if (length < line_end && text[length] != ' ') {
            const auto space = text.rfind(' ', length);
            if (space != 0 && space != std::string_view::npos)
                length = space;
        }
        draw_text(runtime, text.substr(0, length), origin + description_origin * scale, scale,
                  {0.85f, 0.9f, 1});
        text.remove_prefix(length);
        while (text.starts_with(' '))
            text.remove_prefix(1);
        if (text.starts_with('\n'))
            text.remove_prefix(1);
        description_origin.y += 11;
    }
}
} // namespace

MenuLayout::MenuLayout(Vec2 viewport, std::size_t rows) : MenuLayout(viewport, rows, false) {
}

MenuLayout::MenuLayout(Vec2 viewport, std::size_t rows, bool with_preview)
    : visible_rows(std::min(rows, page_size)) {
    if (!finite(viewport) || viewport.x <= 0 || viewport.y <= 0)
        throw std::invalid_argument("Invalid menu viewport");
    const Vec2 list{300, 130.f + static_cast<float>(visible_rows * 40)};
    Vec2 bounds = list;
    Vec2 list_offset{}, preview_offset{};
    if (with_preview && viewport.x >= viewport.y) {
        bounds = {list.x + panel_gap + preview_size.x, std::max(list.y, preview_size.y)};
        list_offset.y = (bounds.y - list.y) / 2;
        preview_offset = {list.x + panel_gap, (bounds.y - preview_size.y) / 2};
    } else if (with_preview) {
        bounds.y += panel_gap + preview_size.y;
        preview_offset = {(list.x - preview_size.x) / 2, list.y + panel_gap};
    }
    scale = std::min({2.f, viewport.x / (bounds.x + 20), viewport.y / (bounds.y + 20)});
    const Vec2 group_origin = (viewport - bounds * scale) / 2;
    origin = group_origin + list_offset * scale;
    if (with_preview)
        preview_origin = group_origin + preview_offset * scale;
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
        has_previews_ = has_previews_ || static_cast<bool>(choice.preview);
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
void LaunchMenu::show_configurations(LaunchSelection selection) {
    const auto &choice = choices_.at(selection.application);
    const auto count = choice.configurations.size();
    if ((count == 0 && selection.configuration != 0) || (count && selection.configuration >= count))
        throw std::out_of_range("Unknown application configuration");
    application_ = selection.application;
    configurations_[application_] = selection.configuration;
    configuring_ = count != 0;
}
MenuLayout LaunchMenu::layout(Vec2 viewport) const {
    return MenuLayout(viewport, row_count(), has_previews_ && !configuring_);
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
    const auto menu_layout = layout(viewport);
    const auto first = selection / MenuLayout::page_size * MenuLayout::page_size;
    // A resting pointer must not undo keyboard navigation on the next frame.
    if (pointer_moved && !input.hit(Key::down) && !input.hit(Key::up)) {
        for (std::size_t i = 0; i < menu_layout.visible_rows && first + i < row_count(); ++i)
            if (menu_layout.contains(i, input.mouse)) {
                selection = first + i;
                break;
            }
    }
    bool confirm = input.hit(Key::enter) || input.hit(Key::space);
    if (input.clicked) {
        if (menu_layout.footer_contains(input.mouse, 150)) {
            exit_requested_ = true;
            return std::nullopt;
        }
        if (row_count() > MenuLayout::page_size) {
            if (menu_layout.footer_contains(input.mouse, 50)) {
                selection = first == 0 ? (row_count() - 1) / MenuLayout::page_size * MenuLayout::page_size
                                       : first - MenuLayout::page_size;
                return std::nullopt;
            }
            if (menu_layout.footer_contains(input.mouse, 250)) {
                selection = first + MenuLayout::page_size < row_count() ? first + MenuLayout::page_size : 0;
                return std::nullopt;
            }
        }
        for (std::size_t i = 0; i < menu_layout.visible_rows && first + i < row_count(); ++i)
            if (menu_layout.contains(i, input.mouse)) {
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
    const auto layout = this->layout(runtime.camera.viewport);
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
    if (layout.preview_origin)
        draw_preview(runtime, choices_[application_], *layout.preview_origin, layout.scale);
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
    selection_ = selection;
    camera = next_camera;
    fresh_ = true;
}
void SessionHost::return_to_menu() {
    session_.reset();
    selection_.reset();
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
            if (request == SessionRequest::menu || request == SessionRequest::configurations) {
                const auto ran = host.selection();
                // Destroy before the menu draws, so a session never coexists with the next one.
                host.return_to_menu();
                if (request == SessionRequest::configurations && ran)
                    menu.show_configurations(*ran);
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
