// Window-free tests for the launcher library (include/kinetra/launcher.h): menu paging
// and configuration, SessionHost transitions, and debug-menu actions and stepper buttons. The loops that
// need a window are covered by runtime_tests.cpp.
#include <kinetra/debug_view.h>
#include <kinetra/launcher.h>

#include <doctest/doctest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace kinetra;
namespace {
/// Session that records the input and time of its last advance, and counts destruction.
struct TestSession : Session {
    int &destroyed;
    double elapsed{-1};
    bool clicked{true};
    explicit TestSession(int &count) : destroyed(count) {}
    ~TestSession() override { ++destroyed; }
    void advance(double dt, const Input &input, Camera2D &) override {
        elapsed = dt;
        clicked = input.clicked;
    }
    void render(Runtime &) override {}
    std::string title() override { return "TEST"; }
};
} // namespace

TEST_CASE("a configurable application asks for its configuration before launching") {
    LaunchMenu menu({{"EXTERNAL", "CUSTOM APP", {"SMALL", "LARGE"}, 1}});
    Input confirm;
    confirm.pressed[std::size_t(Key::enter)] = true;
    CHECK(!menu.update(confirm, {800, 600}));
    CHECK_MESSAGE(menu.configuring(), "configuration skipped");
    const auto selection = menu.update(confirm, {800, 600});
    REQUIRE(selection);
    CHECK_MESSAGE(selection->configuration == 1, "default configuration lost");
}

TEST_CASE("menu pages wrap and stay pickable at every viewport") {
    for (auto viewport : {Vec2{1280, 800}, Vec2{160, 120}}) {
        CAPTURE(viewport.x);
        std::vector<LaunchChoice> entries;
        for (int i = 0; i < 14; ++i)
            entries.push_back({"LAB " + std::to_string(i), "", {}, 0});
        LaunchMenu pages(entries);
        MenuLayout layout(viewport, entries.size());
        Input pointer;
        pointer.clicked = true;
        pointer.mouse = layout.footer_center(250);
        CHECK_MESSAGE((!pages.update(pointer, viewport) && pages.selected() == 6),
                      "next page launched an application or lost selection");
        pages.update(pointer, viewport);
        CHECK_MESSAGE(pages.selected() == 12, "partial last page unreachable");
        pointer.mouse = layout.row_center(1);
        auto last = pages.update(pointer, viewport);
        CHECK_MESSAGE((last && last->application == 13), "last page picking failed");
        Input down;
        down.pressed[std::size_t(Key::down)] = true;
        pages.update(down, viewport);
        CHECK_MESSAGE(pages.selected() == 0, "keyboard wrap failed");
        pointer.mouse = layout.footer_center(50);
        pages.update(pointer, viewport);
        CHECK_MESSAGE(pages.selected() == 12, "previous page wrap failed");
        pointer.mouse = layout.footer_center();
        CHECK_MESSAGE((!pages.update(pointer, viewport) && pages.exit_requested()), "exit button failed");
        CHECK_MESSAGE((layout.origin.x > 0 && layout.origin.y > 0 && layout.footer_center().y < viewport.y),
                      "menu does not fit resized viewport");
    }
}

TEST_CASE("session host isolates transitions and releases sessions") {
    int destroyed = 0;
    bool fail = false;
    SessionHost host(
        {{{"EXTERNAL", "CUSTOM APP", {}, 0}, [&](Camera2D &camera, std::size_t) -> std::unique_ptr<Session> {
              camera.position.x = 42;
              if (fail)
                  throw std::runtime_error("Factory failure");
              return std::make_unique<TestSession>(destroyed);
          }}});
    Camera2D camera;
    host.start({0, 0}, camera);
    auto *session = static_cast<TestSession *>(host.session());
    REQUIRE(session);
    Input click;
    click.clicked = true;
    host.advance(5, click, camera);
    CHECK_MESSAGE((session->elapsed == 0 && !session->clicked), "selection input or time leaked");
    host.advance(0.25, click, camera);
    CHECK_MESSAGE((session->elapsed == 0.25 && session->clicked), "normal input suppressed");
    fail = true;
    camera.position.x = 7;
    CHECK_THROWS_AS(host.start({0, 0}, camera), std::runtime_error);
    CHECK_MESSAGE((host.session() == session && camera.position.x == 7 && destroyed == 0),
                  "failed transition changed session");
    host.return_to_menu();
    CHECK_MESSAGE((!host.session() && destroyed == 1), "return did not release session");
}

TEST_CASE("a debug menu action runs and captures the click") {
    DebugMenu debug;
    debug.visible = true;
    int actions = 0;
    DebugOption action{"RESET", nullptr, [&] { ++actions; }};
    Input click;
    click.clicked = true;
    click.mouse = {1280 - 180 * DebugMenu::scale({1280, 800}), 36 * DebugMenu::scale({1280, 800})};
    CHECK(debug.update(click, {1280, 800}, {&action, 1}));
    CHECK(actions == 1);
}

namespace {
/// A left click at panel position `point` (panel pixels from the menu's top-left corner)
/// in a 1280 × 800 viewport.
Input click_in_debug_menu(const DebugMenu &menu, Vec2 point) {
    const Vec2 viewport{1280, 800};
    Input click;
    click.clicked = true;
    click.mouse = menu.origin(viewport) + point * DebugMenu::scale(viewport);
    return click;
}

/// The vertical middle of the first row, in panel pixels.
constexpr float first_row_middle = DebugMenu::heading_height + DebugMenu::row_height / 2;
} // namespace

TEST_CASE("a debug menu stepper runs the button under the click") {
    DebugMenu debug;
    debug.visible = true;
    int actions = 0, value = 0;
    DebugOption option{"SPEED", nullptr, [&] { ++actions; }};
    option.decrease = [&] { --value; };
    option.increase = [&] { ++value; };
    const float decrease_middle = DebugMenu::decrease_left + DebugMenu::stepper_width / 2;
    const float increase_middle = DebugMenu::increase_left + DebugMenu::stepper_width / 2;
    CHECK(debug.update(click_in_debug_menu(debug, {increase_middle, first_row_middle}), {1280, 800},
                       {&option, 1}));
    CHECK(value == 1);
    debug.update(click_in_debug_menu(debug, {decrease_middle, first_row_middle}), {1280, 800}, {&option, 1});
    debug.update(click_in_debug_menu(debug, {decrease_middle, first_row_middle}), {1280, 800}, {&option, 1});
    CHECK(value == -1);
    CHECK_MESSAGE(actions == 0, "a button click does not also run the row");
    // The gap between the buttons is split between them, and the [+] zone runs to the edge.
    const float gap_left = DebugMenu::decrease_left + DebugMenu::stepper_width;
    debug.update(click_in_debug_menu(debug, {DebugMenu::increase_left - 1, first_row_middle}), {1280, 800},
                 {&option, 1});
    CHECK(value == 0);
    debug.update(click_in_debug_menu(debug, {gap_left + 1, first_row_middle}), {1280, 800}, {&option, 1});
    CHECK(value == -1);
    debug.update(click_in_debug_menu(debug, {DebugMenu::panel_width - 0.5f, first_row_middle}), {1280, 800},
                 {&option, 1});
    CHECK(value == 0);
    CHECK(actions == 0);
}

TEST_CASE("a click beside the stepper buttons still runs the row") {
    DebugMenu debug;
    debug.visible = true;
    bool enabled = false;
    int steps = 0;
    DebugOption option{"WORKLOAD", &enabled};
    option.increase = [&] { ++steps; };
    debug.update(click_in_debug_menu(debug, {DebugMenu::text_margin, first_row_middle}), {1280, 800},
                 {&option, 1});
    CHECK(enabled);
    CHECK(steps == 0);
    // Without a [-] button its zone is part of the row.
    debug.update(click_in_debug_menu(debug, {DebugMenu::decrease_left + 1, first_row_middle}), {1280, 800},
                 {&option, 1});
    CHECK_FALSE(enabled);
    CHECK(steps == 0);
}

TEST_CASE("a row without stepper buttons takes clicks across its full width") {
    DebugMenu debug;
    debug.visible = true;
    int actions = 0;
    DebugOption option{"RESET", nullptr, [&] { ++actions; }};
    for (const float x :
         {0.5f, DebugMenu::decrease_left + 1, DebugMenu::increase_left + 1, DebugMenu::panel_width - 0.5f})
        CHECK(debug.update(click_in_debug_menu(debug, {x, first_row_middle}), {1280, 800}, {&option, 1}));
    CHECK(actions == 4);
}
