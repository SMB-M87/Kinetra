// Window-free tests for kinetra::SettingsOverlay (include/kinetra/settings_view.h).
// The layout cases pin the coordinates Kinetra-Lab's own panel used before the overlay
// moved here (ADR 0013). Every layout case runs with a Demo-like row set (four sim-control
// lines) and a Dotio-like one (three lines), at a large and a tiny viewport.
#include <kinetra/settings_view.h>

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

using kinetra::HelpSection;
using kinetra::Key;
using kinetra::SessionRequest;
using kinetra::SettingsOverlay;
using kinetra::ToggleRow;
using kinetra::Vec2;
using kinetra::ZoomLimits;
using kinetra::ZoomRow;

namespace {
constexpr std::size_t zoom_row = 0, statistics_row = 1, debug_row = 2, sim_row = 3, debug_controls_row = 4;
constexpr float button_column_x = 92;

/// One overlay with its camera and the flags its rows borrow, for one layout.
/// Not copyable: the overlay holds pointers to the flags.
struct Harness {
    bool statistics = false;
    bool debug = false;
    std::size_t sim_lines;
    Vec2 viewport;
    kinetra::Camera2D camera;
    SettingsOverlay overlay;

    Harness(std::size_t lines, Vec2 size)
        : sim_lines(lines), viewport(size),
          overlay({ZoomRow{[] { return ZoomLimits{25, 50}; }}, ToggleRow{"STATISTICS", &statistics},
                   ToggleRow{"DEBUG MODE", &debug},
                   HelpSection{"SIM CONTROLS", std::vector<std::string>(lines, "LINE")},
                   HelpSection{"DEBUG CONTROLS", {"WHEEL  ZOOM", "GHOST CLICK  TELEPORT"}}},
                  &debug) {
        camera.viewport = size;
        camera.pixels_per_unit = 25;
    }
    Harness(const Harness &) = delete;
    Harness &operator=(const Harness &) = delete;

    bool update(const kinetra::Input &input) { return overlay.update(input, camera); }

    /// Clicks at a position in the panel's own layout coordinates (centered modal).
    bool click(float x, float y) {
        kinetra::Input input;
        input.clicked = true;
        input.mouse = viewport / 2 + (Vec2{x, y} - overlay.half_extent()) * SettingsOverlay::scale(viewport);
        return update(input);
    }

    bool press(Key key) {
        kinetra::Input input;
        input.pressed[static_cast<std::size_t>(key)] = true;
        return update(input);
    }

    /// Opens the panel through the SETTINGS toolbar button in the top-left corner.
    bool open_with_button() {
        kinetra::Input input;
        input.clicked = true;
        input.mouse = {10, 10};
        return update(input);
    }
};

/// Runs `check` once per layout, with the layout recorded for failure messages.
void for_each_layout(const std::function<void(Harness &)> &check) {
    for (std::size_t lines : {std::size_t{4}, std::size_t{3}})
        for (auto viewport : {Vec2{1280, 800}, Vec2{160, 120}}) {
            CAPTURE(lines);
            CAPTURE(viewport.x);
            Harness harness(lines, viewport);
            check(harness);
        }
}
} // namespace

TEST_CASE("rows take the coordinates of the former Lab panel") {
    for_each_layout([](Harness &h) {
        CHECK(h.overlay.row_y(zoom_row) == 40);
        CHECK(h.overlay.row_y(statistics_row) == 68);
        CHECK(h.overlay.row_y(debug_row) == 96);
        INFO("a 4-pixel gap separates the first help section from the rows");
        CHECK(h.overlay.row_y(sim_row) == 128);
        CHECK(h.overlay.row_y(debug_controls_row) == 156);
        CHECK(h.overlay.actions_y() == 184);
        CHECK(h.overlay.half_extent().x == 92);
        CHECK(h.overlay.half_extent().y == (184 + 50) / 2.f);
    });
}

TEST_CASE("the scale depends only on the viewport") {
    CHECK(SettingsOverlay::scale({1280, 800}) == 800 / 418.f);
    CHECK(SettingsOverlay::scale({4000, 4000}) == 2);
    CHECK(SettingsOverlay::scale({160, 120}) == 120 / 418.f);
}

TEST_CASE("gameplay keys do not open settings") {
    for_each_layout([](Harness &h) {
        kinetra::Input input;
        input.pressed[static_cast<std::size_t>(Key::space)] = true;
        input.pressed[static_cast<std::size_t>(Key::m)] = true;
        CHECK_FALSE(h.update(input));
        CHECK_FALSE(h.overlay.visible());
        CHECK(h.overlay.request() == SessionRequest::none);

        input = {};
        input.pressed[static_cast<std::size_t>(Key::h)] = true;
        input.pressed[static_cast<std::size_t>(Key::escape)] = true;
        CHECK_FALSE(h.update(input));
    });
}

TEST_CASE("the settings button opens the panel with every section collapsed") {
    for_each_layout([](Harness &h) {
        CHECK(h.open_with_button());
        CHECK(h.overlay.visible());
        CHECK_FALSE(h.overlay.expanded(sim_row));
        CHECK_FALSE(h.overlay.expanded(debug_controls_row));
    });
}

TEST_CASE("help sections expand and collapse independently with fixed spacing") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        const auto collapsed = h.overlay.half_extent();
        const float lines_height = 14.f * static_cast<float>(h.sim_lines - 1);

        h.click(button_column_x, h.overlay.row_y(sim_row));
        CHECK(h.overlay.expanded(sim_row));
        INFO("expanded lines push the next section down by (lines - 1) * 14");
        CHECK(h.overlay.row_y(debug_controls_row) == 156 + lines_height);
        CHECK(h.overlay.half_extent().y == collapsed.y + lines_height / 2);

        h.click(button_column_x, h.overlay.row_y(sim_row));
        CHECK_FALSE(h.overlay.expanded(sim_row));

        h.click(button_column_x, h.overlay.row_y(debug_controls_row));
        CHECK(h.overlay.expanded(debug_controls_row));
        CHECK_FALSE(h.overlay.expanded(sim_row));

        h.click(button_column_x, h.overlay.row_y(sim_row));
        CHECK(h.overlay.expanded(sim_row));
        CHECK(h.overlay.expanded(debug_controls_row));
        const float last_sim_y = 128 + 14.f * static_cast<float>(h.sim_lines);
        CHECK_MESSAGE(h.overlay.row_y(debug_controls_row) - last_sim_y == 14,
                      "sim lines end one line before the next section");
        CHECK_MESSAGE(h.overlay.actions_y() - (h.overlay.row_y(debug_controls_row) + 28) == 28,
                      "the last line keeps the spacing before the action row");

        h.click(button_column_x, h.overlay.row_y(debug_controls_row));
        CHECK_FALSE(h.overlay.expanded(debug_controls_row));
        CHECK_MESSAGE(h.overlay.expanded(sim_row), "collapsing one section keeps the other open");
    });
}

TEST_CASE("zoom buttons stay within the limits") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        CHECK(h.click(56, 46));
        CHECK(h.camera.pixels_per_unit > 25);
        for (int i = 0; i < 30; ++i)
            h.click(56, 46);
        CHECK(h.camera.pixels_per_unit == 50);
        for (int i = 0; i < 30; ++i)
            h.click(124, 46);
        CHECK(h.camera.pixels_per_unit == 25);
    });
}

TEST_CASE("zoom limits are read at click time") {
    bool debug = false;
    kinetra::Camera2D camera;
    camera.pixels_per_unit = 17;
    SettingsOverlay overlay({ZoomRow{[&] { return debug ? ZoomLimits{5, 100} : ZoomLimits{17, 100}; }}});
    kinetra::Input open;
    open.pressed[static_cast<std::size_t>(Key::control)] = true;
    REQUIRE(overlay.update(open, camera));
    kinetra::Input zoom_out;
    zoom_out.clicked = true;
    zoom_out.mouse = camera.viewport / 2 +
                     (Vec2{124, 46} - overlay.half_extent()) * SettingsOverlay::scale(camera.viewport);
    overlay.update(zoom_out, camera);
    CHECK(camera.pixels_per_unit == 17);
    debug = true;
    overlay.update(zoom_out, camera);
    CHECK(camera.pixels_per_unit == doctest::Approx(17 / 1.1f));
}

TEST_CASE("toggle rows flip their borrowed flags") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        h.click(40, 74);
        h.click(40, 102);
        CHECK(h.statistics);
        CHECK(h.debug);
        h.click(40, 74);
        CHECK_FALSE(h.statistics);
    });
}

TEST_CASE("resume closes the panel and captures the click") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        CHECK(h.click(106, h.overlay.actions_y()));
        CHECK_FALSE(h.overlay.visible());
        CHECK(h.overlay.request() == SessionRequest::none);
    });
}

TEST_CASE("launcher and exit buttons raise their requests") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.press(Key::control));
        h.click(50, h.overlay.actions_y());
        CHECK(h.overlay.request() == SessionRequest::menu);
        h.click(146, h.overlay.actions_y());
        CHECK(h.overlay.request() == SessionRequest::exit);
    });
}

TEST_CASE("the settings button does not overlap the debug menu") {
    for_each_layout([](Harness &h) {
        kinetra::Input input;
        input.clicked = true;
        const float debug_scale = kinetra::DebugMenu::scale(h.viewport);
        input.mouse = {h.viewport.x - 180 * debug_scale, 36 * debug_scale};
        CHECK_FALSE(h.update(input));
        CHECK_FALSE(h.overlay.visible());
    });
}

TEST_CASE("ctrl toggles settings and captures the frames it is open") {
    for_each_layout([](Harness &h) {
        CHECK(h.press(Key::control));
        CHECK(h.overlay.visible());
        CHECK_MESSAGE(h.press(Key::control), "the closing frame is still captured");
        CHECK_FALSE(h.overlay.visible());
        CHECK_FALSE(h.press(Key::space));
    });
}

TEST_CASE("the debug button opens captures and closes the debug menu") {
    SettingsOverlay overlay({});
    const Vec2 viewport{160, 120};
    kinetra::DebugMenu menu;
    bool enabled = false;
    std::array<kinetra::DebugOption, 1> options{{{"TEST", &enabled}}};
    kinetra::Input input;
    input.clicked = input.mouse_down = true;
    input.mouse = {80, 12};
    CHECK(overlay.update_debug(input, viewport, menu, options));
    CHECK(menu.visible);
    CHECK_FALSE(enabled);

    INFO("the debug menu leaves room for the toolbar");
    CHECK(menu.origin(viewport).x == 8);
    CHECK(menu.origin(viewport).y == 32);

    input.clicked = false;
    CHECK(overlay.update_debug(input, viewport, menu, options));
    CHECK_MESSAGE(menu.visible, "holding the button must not repeat the toggle");

    input.clicked = true;
    input.mouse = menu.origin(viewport) + Vec2{10, 28} * kinetra::DebugMenu::scale(viewport);
    CHECK(overlay.update_debug(input, viewport, menu, options));
    CHECK_MESSAGE(enabled, "picking follows the upper-left menu");

    input.mouse = {110, 12};
    CHECK(overlay.update_debug(input, viewport, menu, options));
    CHECK_FALSE(menu.visible);
}

TEST_CASE("a panel without help sections puts the action row after the last row") {
    bool flag = false;
    SettingsOverlay overlay({ZoomRow{[] { return ZoomLimits{1, 2}; }}, ToggleRow{"FLAG", &flag}});
    CHECK(overlay.row_y(1) == 68);
    CHECK(overlay.actions_y() == 96);
}

TEST_CASE("invalid rows are rejected") {
    bool flag = false;
    CHECK_THROWS_AS(SettingsOverlay({HelpSection{"HELP", {}}, ToggleRow{"FLAG", &flag}}),
                    std::invalid_argument);
    CHECK_THROWS_AS(SettingsOverlay({ToggleRow{"FLAG", nullptr}}), std::invalid_argument);
    CHECK_THROWS_AS(SettingsOverlay({ZoomRow{}}), std::invalid_argument);
    SettingsOverlay overlay({ToggleRow{"FLAG", &flag}});
    CHECK_THROWS_AS(static_cast<void>(overlay.row_y(1)), std::out_of_range);
}
