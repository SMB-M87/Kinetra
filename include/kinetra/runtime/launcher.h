#pragma once

#include <functional>
#include <kinetra/runtime/session.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace kinetra {
/// Window title run_launcher() shows while the menu is open. A running session supplies
/// its own name through Session::title().
inline constexpr std::string_view launcher_window_title = "Launcher";
/// One application entry of the launcher menu.
struct LaunchChoice {
    /// Row label; must not be empty.
    std::string name;
    /// Text for the main menu's preview panel. Wraps to four lines when previews are enabled.
    std::string description;
    /// Names of the variants shown on the second menu page; empty skips that page. The
    /// names mean nothing to the engine: the application factory interprets the index.
    std::vector<std::string> configurations;
    /// Index into `configurations` selected first; zero when there are none.
    std::size_t default_configuration{};
    /// Optional still shown when this application is highlighted. One image enables
    /// the preview panel for the whole application list.
    std::shared_ptr<const RgbaImage> preview{};
};
/// Application and configuration index picked in the menu.
struct LaunchSelection {
    std::size_t application{}, configuration{};
};
/// Shared logical-pixel geometry for both rendering and picking.
///
/// The list is 300 panel pixels wide, with a heading, up to page_size rows of 40 pixels,
/// and a footer. It is centered with its preview panel when present. Scale is at most 2x.
struct MenuLayout {
    /// Top-left corner of the panel in logical window pixels.
    Vec2 origin;
    /// Screen pixels per panel pixel.
    float scale;
    /// Layout for `rows` rows in a viewport of logical window pixels. Throws
    /// std::invalid_argument for a nonfinite or nonpositive viewport.
    explicit MenuLayout(Vec2 viewport, std::size_t rows = 3);
    /// Center of visible row `index` (0 = top of the page) in window pixels.
    Vec2 row_center(std::size_t index) const;
    /// Whether `point` (window pixels) hits visible row `index`.
    bool contains(std::size_t index, Vec2 point) const;
    /// Rows per page.
    static constexpr std::size_t page_size = 6;
    /// Rows on screen: the row count, at most page_size.
    std::size_t visible_rows;
    /// Top-left of the optional preview panel in logical window pixels.
    std::optional<Vec2> preview_origin;
    /// Center of the footer button at panel x-coordinate `x` (EXIT 150, PREVIOUS 50,
    /// NEXT 250), in window pixels.
    Vec2 footer_center(float x = 150) const;
    /// Whether `point` (window pixels) hits the footer button at panel x-coordinate `x`.
    bool footer_contains(Vec2 point, float x) const;

  private:
    friend class LaunchMenu;
    MenuLayout(Vec2 viewport, std::size_t rows, bool with_preview);
};
/// Value-owned registration metadata; configuration names have no engine semantics.
///
/// Two pages: the application list and, for an application with configurations, its
/// configuration list. Up/Down, the pointer and Enter/Space or a click select; M returns
/// from the configuration page. Each application remembers its last configuration.
class LaunchMenu {
  public:
    /// Throws std::invalid_argument for an empty list, an empty name or a default
    /// configuration out of range.
    explicit LaunchMenu(std::vector<LaunchChoice> choices, std::string heading = "KINETRA LAUNCHER");
    /// Applies one frame of input; `viewport` in logical window pixels. Returns the
    /// selection when the user confirms a final row, std::nullopt otherwise.
    std::optional<LaunchSelection> update(const Input &input, Vec2 viewport);
    /// Highlighted row index within the current page's list.
    std::size_t selected() const;
    /// True once EXIT was clicked; never reset.
    bool exit_requested() const { return exit_requested_; }
    /// Whether the configuration page is showing.
    bool configuring() const { return configuring_; }
    /// Shows the configuration page of `selection.application` with
    /// `selection.configuration` selected, as a host does when a session asks for
    /// SessionRequest::configurations. For an application without configurations it shows
    /// the application list with that application selected. The next update() takes the
    /// pointer as resting where it finds it, so a pointer that the session left over another
    /// row does not replace the selection until it moves. Throws std::out_of_range for an
    /// unknown application or configuration, and changes nothing then.
    void show_configurations(LaunchSelection selection);
    /// Configuration currently chosen for the highlighted application.
    std::size_t configuration() const { return configurations_[application_]; }
    /// Number of rows in the current list.
    std::size_t row_count() const;
    /// Label of `row` in the current list; throws std::out_of_range for an invalid row.
    /// The view stays valid as long as the menu.
    std::string_view row_name(std::size_t row) const;
    /// Description of the highlighted application; valid as long as the menu.
    std::string_view description() const;
    /// Layout of the current page in logical window pixels, shared by drawing and picking.
    MenuLayout layout(Vec2 viewport) const;
    /// Draws a whole frame (begin() to end()) showing the menu, with an optional status
    /// line of at most 80 characters at the bottom.
    void draw(Runtime &runtime, std::string_view status = {}) const;

  private:
    std::vector<LaunchChoice> choices_;
    std::string heading_;
    std::vector<std::size_t> configurations_;
    std::size_t application_{};
    bool configuring_{}, exit_requested_{};
    bool has_previews_{};
    Vec2 pointer_{-1, -1};
    /// Set by show_configurations: the next update() records the pointer without hovering.
    bool settle_pointer_{};
};
/// An application the launcher can start.
struct ApplicationRegistration {
    /// Menu entry and configuration names.
    LaunchChoice choice;
    /// Creates the session for a configuration index; may move or zoom the camera it is
    /// given. Must not return null. Exceptions propagate to the caller of start().
    std::function<std::unique_ptr<Session>(Camera2D &, std::size_t)> create;
};
/// Optional host for standard or custom launchers. Factory failure leaves the old
/// session and camera intact; successful transitions suppress selection input/time.
///
/// Owns at most one session. run_launcher() starts sessions only from the menu, after the
/// previous one was destroyed, so two sessions never exist at the same time there.
class SessionHost {
  public:
    /// Throws std::invalid_argument for invalid choices (see LaunchMenu) or a missing factory.
    explicit SessionHost(std::vector<ApplicationRegistration> applications);
    /// Copies of the registered menu entries, in registration order.
    std::vector<LaunchChoice> choices() const;
    /// Creates the selected session on a copy of `camera`, then replaces the current session
    /// and camera. Throws std::out_of_range for an unknown application or configuration,
    /// std::runtime_error when the factory returns null, and passes factory exceptions on;
    /// in all these cases nothing changes.
    void start(LaunchSelection selection, Camera2D &camera);
    /// Destroys the current session, if any.
    void return_to_menu();
    /// The running session, or null in the menu. Owned by the host.
    Session *session() const { return session_.get(); }
    /// Application and configuration of the running session; std::nullopt in the menu.
    std::optional<LaunchSelection> selection() const { return selection_; }
    /// Advances the running session. Its first advance after start() gets zero elapsed time
    /// and empty input, so the click that started it is not replayed. No-op in the menu.
    void advance(double elapsed, const Input &input, Camera2D &camera);

  private:
    std::vector<ApplicationRegistration> applications_;
    std::unique_ptr<Session> session_;
    std::optional<LaunchSelection> selection_;
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
/// the menu in the same frame, `configurations` does the same on the configuration page of
/// the application that ran (LaunchMenu::show_configurations), and `exit` ends the loop
/// without rendering. `bindings` adds the menu key and the Escape policy. Factories own
/// application configuration. Zero frame limit means unlimited; returns the frames
/// presented. Throws std::invalid_argument for a negative frame limit; factory exceptions
/// propagate.
///
/// `initial` starts that session immediately instead of showing the menu. The loop also
/// ends when the window closes or EXIT is clicked in the menu.
int run_launcher(Runtime &runtime, std::vector<ApplicationRegistration> applications,
                 std::optional<LaunchSelection> initial = std::nullopt, int frame_limit = 0,
                 LauncherBindings bindings = {});
} // namespace kinetra
