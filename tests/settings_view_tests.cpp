// Window-free tests for kinetra::SettingsOverlay (include/kinetra/runtime/settings_view.h): layout,
// rows, the Ctrl and Shift+Ctrl shortcuts, and the debug menu it shows and hides. Every
// layout case runs with a row set of four help lines and one of three, at a large and a
// tiny viewport.
#include <kinetra/runtime/inspector_view.h>
#include <kinetra/runtime/settings_view.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

using kinetra::DebugEntry;
using kinetra::DebugMenu;
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
/// A panel x coordinate on the label of every row, and one beside the zoom buttons.
constexpr float label_x = 60;
/// Distance from a row's position, the top of its text, to a point well inside its band.
constexpr float into_row = 3;

/// Input with `keys` going down this frame and staying down.
kinetra::Input pressing(std::initializer_list<Key> keys, std::initializer_list<Key> already_held = {}) {
    kinetra::Input input;
    for (const Key key : keys)
        input.pressed[static_cast<std::size_t>(key)] = input.held[static_cast<std::size_t>(key)] = true;
    for (const Key key : already_held)
        input.held[static_cast<std::size_t>(key)] = true;
    return input;
}

/// Input with `keys` held, none of them newly pressed.
kinetra::Input holding(std::initializer_list<Key> keys) {
    return pressing({}, keys);
}

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

    /// Clicks the label of row `index`.
    bool click_row(std::size_t index) { return click(label_x, overlay.row_y(index) + into_row); }

    /// A key that goes down and up within one frame, as scripted input sends it.
    bool tap(Key key) {
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

/// An overlay with a debug flag and a debug menu of one check row, for the shortcut cases.
struct DebugHarness {
    bool debug = false;
    bool layer = false;
    Vec2 viewport{1280, 800};
    kinetra::Camera2D camera;
    SettingsOverlay overlay{{ToggleRow{"DEBUG MODE", &debug}}, &debug};
    DebugMenu menu;
    std::vector<DebugEntry> entries{kinetra::debug_check("LAYER", layer, 0)};

    DebugHarness() { camera.viewport = viewport; }
    DebugHarness(const DebugHarness &) = delete;
    DebugHarness &operator=(const DebugHarness &) = delete;

    /// One frame as a session runs it: the panel first, then the debug menu. Returns whether
    /// the debug menu or its button captured the pointer.
    bool frame(const kinetra::Input &input) {
        overlay.update(input, camera);
        return overlay.update_debug(input, viewport, menu, entries);
    }
};
} // namespace

TEST_CASE("rows are evenly spaced and help sections follow a gap") {
    for_each_layout([](Harness &h) {
        CHECK(h.overlay.row_y(zoom_row) == 33);
        CHECK(h.overlay.row_y(statistics_row) == 51);
        CHECK(h.overlay.row_y(debug_row) == 69);
        INFO("a 4-pixel gap separates the first help section from the rows");
        CHECK(h.overlay.row_y(sim_row) == 91);
        CHECK(h.overlay.row_y(debug_controls_row) == 109);
        CHECK(h.overlay.actions_y() == 139);
        const auto half_extent = h.overlay.half_extent();
        INFO("half extent (", half_extent.x, ", ", half_extent.y, ")");
        CHECK(half_extent == Vec2{SettingsOverlay::panel_width / 2, (139 + 22) / 2.f});
    });
}

TEST_CASE("the scale depends only on the viewport") {
    CHECK(SettingsOverlay::scale({1280, 800}) == 2);
    CHECK(SettingsOverlay::scale({960, 540}) == 540 / 315.f);
    CHECK(SettingsOverlay::scale({160, 120}) == 120 / 315.f);
    CHECK(SettingsOverlay::scale({200, 800}) == 200 / 228.f);
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
        input.pressed[static_cast<std::size_t>(Key::backquote)] = true;
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

TEST_CASE("help sections expand and collapse independently and push later rows down") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        const auto collapsed = h.overlay.half_extent();
        // Every line takes 12 pixels, and 4 more separate the last one from the next row.
        const float lines_height = 12.f * static_cast<float>(h.sim_lines) + 4;

        h.click_row(sim_row);
        CHECK(h.overlay.expanded(sim_row));
        CHECK(h.overlay.row_y(debug_controls_row) == 109 + lines_height);
        CHECK(h.overlay.half_extent().y == collapsed.y + lines_height / 2);

        h.click_row(sim_row);
        CHECK_FALSE(h.overlay.expanded(sim_row));
        CHECK(h.overlay.half_extent().y == collapsed.y);

        h.click_row(debug_controls_row);
        CHECK(h.overlay.expanded(debug_controls_row));
        CHECK_FALSE(h.overlay.expanded(sim_row));

        h.click_row(sim_row);
        CHECK(h.overlay.expanded(sim_row));
        CHECK(h.overlay.expanded(debug_controls_row));
        CHECK_MESSAGE(h.overlay.actions_y() == 139 + lines_height + 2 * 12 + 4,
                      "both sections' lines sit above the action row");

        h.click_row(debug_controls_row);
        CHECK_FALSE(h.overlay.expanded(debug_controls_row));
        CHECK_MESSAGE(h.overlay.expanded(sim_row), "collapsing one section keeps the other open");
    });
}

TEST_CASE("a row is clickable across the panel within its band") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        const float y = h.overlay.row_y(statistics_row);
        h.click(2, y - 4);
        CHECK(h.statistics);
        h.click(SettingsOverlay::panel_width - 2, y + 12);
        CHECK_FALSE(h.statistics);
        INFO("one pixel above the band belongs to the row before it, the zoom row");
        h.click(label_x, y - 6);
        CHECK_FALSE(h.statistics);
        INFO("a click beside the panel is captured and does nothing");
        CHECK(h.click(-4, y));
        CHECK_FALSE(h.statistics);
    });
}

TEST_CASE("zoom buttons stay within the limits") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        const float y = h.overlay.row_y(zoom_row) + into_row;
        CHECK(h.click(SettingsOverlay::zoom_in_x, y));
        CHECK(h.camera.pixels_per_unit > 25);
        for (int i = 0; i < 30; ++i)
            h.click(SettingsOverlay::zoom_in_x, y);
        CHECK(h.camera.pixels_per_unit == 50);
        INFO("the label is not a button");
        h.click(label_x, y);
        CHECK(h.camera.pixels_per_unit == 50);
        for (int i = 0; i < 30; ++i)
            h.click(SettingsOverlay::zoom_out_x, y);
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
    zoom_out.mouse = camera.viewport / 2 + (Vec2{SettingsOverlay::zoom_out_x, overlay.row_y(0) + into_row} -
                                            overlay.half_extent()) *
                                               SettingsOverlay::scale(camera.viewport);
    overlay.update(zoom_out, camera);
    CHECK(camera.pixels_per_unit == 17);
    debug = true;
    overlay.update(zoom_out, camera);
    CHECK(camera.pixels_per_unit == doctest::Approx(17 / 1.1f));
}

TEST_CASE("toggle rows flip their borrowed flags") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        h.click_row(statistics_row);
        h.click_row(debug_row);
        CHECK(h.statistics);
        CHECK(h.debug);
        h.click_row(statistics_row);
        CHECK_FALSE(h.statistics);
    });
}

TEST_CASE("resume closes the panel and captures the click") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.open_with_button());
        CHECK(h.click(SettingsOverlay::resume_x, h.overlay.actions_y() + into_row));
        CHECK_FALSE(h.overlay.visible());
        CHECK(h.overlay.request() == SessionRequest::none);
    });
}

TEST_CASE("launcher and exit buttons raise their requests") {
    for_each_layout([](Harness &h) {
        REQUIRE(h.tap(Key::control));
        INFO("the space between two buttons is not a button");
        h.click((SettingsOverlay::launcher_x + SettingsOverlay::resume_x) / 2,
                h.overlay.actions_y() + into_row);
        CHECK(h.overlay.request() == SessionRequest::none);
        CHECK(h.overlay.visible());
        h.click(SettingsOverlay::launcher_x, h.overlay.actions_y() + into_row);
        CHECK(h.overlay.request() == SessionRequest::menu);
        h.click(SettingsOverlay::exit_x, h.overlay.actions_y() + into_row);
        CHECK(h.overlay.request() == SessionRequest::exit);
    });
}

TEST_CASE("the settings button does not overlap the debug menu") {
    for_each_layout([](Harness &h) {
        kinetra::Input input;
        input.clicked = true;
        const float debug_scale = DebugMenu::scale(h.viewport);
        input.mouse = {h.viewport.x - 180 * debug_scale, 36 * debug_scale};
        CHECK_FALSE(h.update(input));
        CHECK_FALSE(h.overlay.visible());
    });
}

TEST_CASE("a ctrl tap toggles settings and captures the frames it is open") {
    for_each_layout([](Harness &h) {
        CHECK(h.tap(Key::control));
        CHECK(h.overlay.visible());
        CHECK_MESSAGE(h.tap(Key::control), "the closing frame is still captured");
        CHECK_FALSE(h.overlay.visible());
        CHECK_FALSE(h.tap(Key::space));
    });
}

TEST_CASE("a held ctrl opens settings when it is released") {
    for_each_layout([](Harness &h) {
        CHECK_FALSE(h.update(pressing({Key::control})));
        CHECK_FALSE(h.update(holding({Key::control})));
        CHECK_FALSE(h.overlay.visible());
        CHECK(h.update({}));
        CHECK(h.overlay.visible());
        CHECK_MESSAGE(h.update({}), "one release toggles once");
        CHECK(h.overlay.visible());
    });
}

TEST_CASE("ctrl that a focus loss released does not open settings") {
    for_each_layout([](Harness &h) {
        h.update(pressing({Key::control}));
        // The runtime releases every key when the window loses focus.
        kinetra::Input unfocused;
        unfocused.focus_lost = true;
        CHECK_FALSE(h.update(unfocused));
        CHECK_FALSE(h.update({}));
        CHECK_FALSE(h.overlay.visible());
        INFO("the next tap works again");
        CHECK(h.tap(Key::control));
    });
}

TEST_CASE("ctrl with shift never opens settings in either order") {
    for_each_layout([](Harness &h) {
        // Shift first.
        h.update(pressing({Key::shift}));
        h.update(pressing({Key::control}, {Key::shift}));
        h.update(holding({Key::shift}));
        h.update({});
        CHECK_FALSE(h.overlay.visible());
        // Ctrl first, then Shift joins while Ctrl is still down.
        h.update(pressing({Key::control}));
        h.update(pressing({Key::shift}, {Key::control}));
        h.update(holding({Key::shift}));
        h.update({});
        CHECK_FALSE(h.overlay.visible());
        // Both in one frame.
        h.update(pressing({Key::shift, Key::control}));
        h.update({});
        CHECK_FALSE(h.overlay.visible());
        INFO("ctrl alone still works afterwards");
        CHECK(h.tap(Key::control));
        CHECK(h.overlay.visible());
    });
}

TEST_CASE("the debug chord is the frame the second key goes down") {
    CHECK_FALSE(kinetra::debug_chord({}));
    CHECK_FALSE(kinetra::debug_chord(pressing({Key::control})));
    CHECK_FALSE(kinetra::debug_chord(pressing({Key::shift})));
    CHECK(kinetra::debug_chord(pressing({Key::control}, {Key::shift})));
    CHECK(kinetra::debug_chord(pressing({Key::shift}, {Key::control})));
    CHECK(kinetra::debug_chord(pressing({Key::shift, Key::control})));
    CHECK_MESSAGE(!kinetra::debug_chord(holding({Key::shift, Key::control})), "holding both does not repeat");
    kinetra::Input tapped;
    tapped.pressed[static_cast<std::size_t>(Key::shift)] =
        tapped.pressed[static_cast<std::size_t>(Key::control)] = true;
    CHECK_MESSAGE(kinetra::debug_chord(tapped), "keys pressed and released within the frame count");
}

TEST_CASE("shift ctrl switches the debug mode on and shows the menu then hides it") {
    DebugHarness h;
    h.frame(pressing({Key::shift}));
    CHECK_FALSE(h.menu.visible);
    h.frame(pressing({Key::control}, {Key::shift}));
    CHECK(h.debug);
    CHECK(h.menu.visible);
    CHECK_FALSE(h.overlay.visible());
    h.frame(holding({Key::shift, Key::control}));
    CHECK_MESSAGE(h.menu.visible, "holding the keys does not repeat the toggle");
    h.frame({});
    h.frame(pressing({Key::shift}, {Key::control}));
    CHECK_FALSE(h.menu.visible);
    CHECK_MESSAGE(h.debug, "hiding the menu leaves the debug mode on");
}

TEST_CASE("shift ctrl shows a hidden menu when the debug mode is already on") {
    DebugHarness h;
    h.debug = true;
    h.frame(pressing({Key::shift, Key::control}));
    CHECK(h.menu.visible);
    CHECK(h.debug);
}

TEST_CASE("the debug menu is hidden while the debug mode is off") {
    DebugHarness h;
    h.debug = true;
    h.menu.visible = true;
    kinetra::Input over_menu;
    over_menu.mouse = Vec2{8, 32} + Vec2{20, DebugMenu::row_top(0) + 4} * DebugMenu::scale(h.viewport);
    CHECK(h.frame(over_menu));
    h.debug = false;
    CHECK_FALSE(h.frame(over_menu));
    CHECK_FALSE(h.menu.visible);
    INFO("the DEBUG button is not there either");
    kinetra::Input on_button;
    on_button.clicked = true;
    on_button.mouse = {80, 12};
    CHECK_FALSE(h.frame(on_button));
    CHECK_FALSE(h.menu.visible);
}

TEST_CASE("a debug menu that the debug mode hides discards a number being typed") {
    DebugHarness h;
    h.debug = true;
    h.menu.visible = true;
    h.entries = {{.label = "RATE", .set = [](std::uint64_t) { FAIL("a discarded number was applied"); }}};
    kinetra::Input typing;
    typing.mouse = Vec2{8, 32} + Vec2{20, DebugMenu::row_top(0) + 4} * DebugMenu::scale(h.viewport);
    typing.text = "5";
    h.frame(typing);
    REQUIRE(h.menu.typing());
    h.debug = false;
    h.frame({});
    CHECK_FALSE(h.menu.typing());
    CHECK_FALSE(h.menu.keys_captured());
    h.debug = true;
    h.menu.visible = true;
    kinetra::Input enter;
    enter.mouse = typing.mouse;
    enter.pressed[static_cast<std::size_t>(Key::enter)] = true;
    h.frame(enter);
}

TEST_CASE("the settings panel hides the debug menu and takes its shortcut") {
    DebugHarness h;
    h.debug = true;
    h.menu.visible = true;
    kinetra::Input open;
    open.pressed[static_cast<std::size_t>(Key::control)] = true;
    CHECK_FALSE(h.frame(open));
    CHECK(h.overlay.visible());
    CHECK_FALSE(h.menu.visible);
    h.frame(pressing({Key::shift, Key::control}));
    CHECK_MESSAGE(!h.menu.visible, "the shortcut does nothing behind the panel");
    CHECK(h.overlay.visible());
}

TEST_CASE("an overlay without a debug flag has no debug menu") {
    SettingsOverlay overlay({});
    kinetra::Camera2D camera;
    DebugMenu menu;
    menu.visible = true;
    std::vector<DebugEntry> entries;
    const auto chord = pressing({Key::shift, Key::control});
    overlay.update(chord, camera);
    CHECK_FALSE(overlay.update_debug(chord, camera.viewport, menu, entries));
    CHECK_FALSE(menu.visible);
}

TEST_CASE("the debug button opens captures and closes the debug menu") {
    DebugHarness h;
    h.debug = true;
    h.viewport = {160, 120};
    h.camera.viewport = h.viewport;
    kinetra::Input input;
    input.clicked = input.mouse_down = true;
    input.mouse = {80, 12};
    CHECK(h.frame(input));
    CHECK(h.menu.visible);
    CHECK_FALSE(h.layer);

    INFO("the debug menu leaves room for the toolbar");
    const auto origin = h.menu.origin(h.viewport);
    INFO("origin (", origin.x, ", ", origin.y, ")");
    CHECK(origin == Vec2{8, 32});

    input.clicked = false;
    CHECK(h.frame(input));
    CHECK_MESSAGE(h.menu.visible, "holding the button must not repeat the toggle");

    input.clicked = true;
    input.mouse = h.menu.origin(h.viewport) +
                  Vec2{DebugMenu::label_left(0), DebugMenu::row_top(0) + 6} * DebugMenu::scale(h.viewport);
    CHECK(h.frame(input));
    CHECK_MESSAGE(h.layer, "picking follows the upper-left menu");

    input.mouse = {110, 12};
    CHECK(h.frame(input));
    CHECK_FALSE(h.menu.visible);
}

TEST_CASE("a panel without help sections puts the action row after the last row") {
    bool flag = false;
    SettingsOverlay overlay({ZoomRow{[] { return ZoomLimits{1, 2}; }}, ToggleRow{"FLAG", &flag}});
    CHECK(overlay.row_y(1) == 51);
    CHECK(overlay.actions_y() == 81);
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

TEST_CASE("inspector layout wraps long values and snaps its input bounds inward") {
    kinetra::InspectorView inspector;
    const kinetra::Aabb area{{266.45f, 8.25f}, {530.75f, 401.75f}};
    kinetra::InspectorSnapshot snapshot{"MOVER", {{"", {{"ROUTE", "A"}}}}};
    const auto short_box = inspector.bounds(snapshot, area);
    snapshot.groups[0].rows[0].value.assign(96, 'A');
    const auto long_box = inspector.bounds(snapshot, area);
    CHECK(long_box.max.y - short_box.max.y == 4 * 13);
    INFO("minimum (", long_box.min.x, ", ", long_box.min.y, ")");
    CHECK(long_box.min == Vec2{267, 9});
    CHECK(long_box.max.x == 530);
    kinetra::Input input;
    input.clicked = true;
    input.mouse = {long_box.min.x - 1, 18};
    CHECK_FALSE(inspector.update(input, snapshot, area).captured);
    input.mouse.x += 1;
    CHECK(inspector.update(input, snapshot, area).captured);
}
