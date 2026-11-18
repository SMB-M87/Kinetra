#pragma once

#include <functional>
#include <kinetra/session.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace kinetra {
struct LaunchChoice {
    std::string name;
    std::string description;
    std::vector<std::string> configurations;
    std::size_t default_configuration{};
};
struct LaunchSelection {
    std::size_t application{}, configuration{};
};
/// Shared logical-pixel geometry for both rendering and picking.
struct MenuLayout {
    Vec2 origin;
    float scale;
    explicit MenuLayout(Vec2 viewport, std::size_t rows = 3);
    Vec2 row_center(std::size_t index) const;
    bool contains(std::size_t index, Vec2 point) const;
    static constexpr std::size_t page_size = 6;
    std::size_t visible_rows;
    Vec2 footer_center(float x = 150) const;
    bool footer_contains(Vec2 point, float x) const;
};
/// Value-owned registration metadata; configuration names have no engine semantics.
class LaunchMenu {
  public:
    explicit LaunchMenu(std::vector<LaunchChoice> choices, std::string heading = "KINETRA LAUNCHER");
    std::optional<LaunchSelection> update(const Input &input, Vec2 viewport);
    std::size_t selected() const;
    bool exit_requested() const { return exit_requested_; }
    bool configuring() const { return configuring_; }
    std::size_t configuration() const { return configurations_[application_]; }
    std::size_t row_count() const;
    std::string_view row_name(std::size_t row) const;
    std::string_view description() const;
    void draw(Runtime &runtime, std::string_view status = {}) const;

  private:
    std::vector<LaunchChoice> choices_;
    std::string heading_;
    std::vector<std::size_t> configurations_;
    std::size_t application_{};
    bool configuring_{}, exit_requested_{};
    Vec2 pointer_{-1, -1};
};
struct ApplicationRegistration {
    LaunchChoice choice;
    std::function<std::unique_ptr<Session>(Camera2D &, std::size_t)> create;
};
/// Optional host for standard or custom launchers. Factory failure leaves the old
/// session and camera intact; successful transitions suppress selection input/time.
class SessionHost {
  public:
    explicit SessionHost(std::vector<ApplicationRegistration> applications);
    std::vector<LaunchChoice> choices() const;
    void start(LaunchSelection selection, Camera2D &camera);
    void return_to_menu();
    Session *session() const { return session_.get(); }
    void advance(double elapsed, const Input &input, Camera2D &camera);

  private:
    std::vector<ApplicationRegistration> applications_;
    std::unique_ptr<Session> session_;
    bool fresh_{};
};
/// Keys the standard launcher loop handles itself; sessions receive all other input.
struct LauncherBindings {
    /// Returns from a running session to the menu. std::nullopt leaves leaving to
    /// SessionRequest::menu alone.
    std::optional<Key> menu_key = Key::m;
    /// Whether Escape closes the window while the menu shows. During a run,
    /// Session::close_on_escape() decides instead.
    bool escape_closes_menu = true;
};
/// Standard menu/session event loop.
///
/// After every advance it reads Session::request(): `menu` destroys the session and draws
/// the menu in the same frame, `exit` ends the loop without rendering. `bindings` adds the
/// menu key and the Escape policy. Factories own application configuration. Zero frame
/// limit means unlimited; returns the frames presented. Throws std::invalid_argument for a
/// negative frame limit; factory exceptions propagate.
int run_launcher(Runtime &runtime, std::vector<ApplicationRegistration> applications,
                 std::optional<LaunchSelection> initial = std::nullopt, int frame_limit = 0,
                 LauncherBindings bindings = {});
} // namespace kinetra
