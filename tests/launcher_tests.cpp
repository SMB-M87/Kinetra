// Window-free tests for the launcher library (include/kinetra/runtime/launcher.h): menu paging
// and configuration, SessionHost transitions, and debug-menu actions and stepper buttons. The loops that
// need a window are covered by runtime_tests.cpp.
#include <kinetra/runtime/debug_view.h>
#include <kinetra/runtime/launcher.h>

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

TEST_CASE("preview layout follows selection and only applies to the application list") {
    const std::uint8_t pixel[]{0, 0, 0, 255};
    const auto image = std::make_shared<const RgbaImage>(1, 1, pixel);
    LaunchMenu menu({{"ONE", "FIRST", {}, 0, image}, {"TWO", "SECOND", {"SMALL"}, 0}});
    const Vec2 wide{1280, 800}, tall{360, 620};
    const auto landscape = menu.layout(wide);
    const auto portrait = menu.layout(tall);
    REQUIRE(landscape.preview_origin);
    REQUIRE(portrait.preview_origin);
    CHECK(landscape.preview_origin->x > landscape.row_center(0).x);
    CHECK(portrait.preview_origin->y > portrait.footer_center().y);
    Input input;
    input.mouse = landscape.row_center(1);
    CHECK_FALSE(menu.update(input, wide));
    CHECK(menu.description() == "SECOND");
    CHECK(menu.layout(wide).row_center(1).x == landscape.row_center(1).x);
    input.pressed[std::size_t(Key::up)] = true;
    menu.update(input, wide);
    CHECK(menu.description() == "FIRST");
    input.pressed = {};
    input.clicked = true;
    CHECK_FALSE(menu.update(input, wide));
    CHECK(menu.configuring());
    CHECK_FALSE(menu.layout(wide).preview_origin);
    input.mouse = menu.layout(wide).row_center(0);
    const auto picked = menu.update(input, wide);
    REQUIRE(picked);
    CHECK(picked->application == 1);
    CHECK(menu.layout(wide).preview_origin);
    CHECK_FALSE(LaunchMenu({{"PLAIN", "NO IMAGE", {}, 0}}).layout(wide).preview_origin);
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

namespace {
const Vec2 menu_viewport{1280, 800};

/// A left click at panel position `point` (panel pixels from the menu's top-left corner)
/// in a 1280 × 800 viewport.
Input click_in_debug_menu(const DebugMenu &menu, Vec2 point) {
    Input click;
    click.clicked = true;
    click.mouse = menu.origin(menu_viewport) + point * DebugMenu::scale(menu_viewport);
    return click;
}

/// The vertical middle of row `row`, in panel pixels.
constexpr float row_middle(std::size_t row) {
    return DebugMenu::heading_height + (static_cast<float>(row) + 0.5f) * DebugMenu::row_height;
}

/// A menu that shows, with the entries it is tested with.
struct Menu {
    DebugMenu debug;
    std::vector<DebugEntry> entries;
    explicit Menu(std::vector<DebugEntry> rows) : entries(std::move(rows)) { debug.visible = true; }
    /// Clicks panel position {x, middle of `row`}; returns whether the menu captured it.
    bool click(float x, std::size_t row = 0) {
        return debug.update(click_in_debug_menu(debug, {x, row_middle(row)}), menu_viewport, entries);
    }
};
} // namespace

TEST_CASE("a debug menu action runs and captures the click") {
    int actions = 0;
    Menu menu({{.label = "RESET", .action = [&] { ++actions; }}});
    CHECK(menu.click(DebugMenu::panel_width / 2));
    CHECK(actions == 1);
}

TEST_CASE("a click reaches the row it is on") {
    int first = 0, second = 0, third = 0;
    Menu menu({{.label = "FIRST", .action = [&] { ++first; }},
               {.label = "SECOND", .action = [&] { ++second; }, .depth = 1},
               {.label = "THIRD", .action = [&] { ++third; }, .depth = 2}});
    menu.click(DebugMenu::label_left(1), 1);
    menu.click(DebugMenu::label_left(2), 2);
    menu.click(DebugMenu::label_left(2), 2);
    CHECK(first == 0);
    CHECK(second == 1);
    CHECK(third == 2);
}

TEST_CASE("the title bar and the space below the last row capture without acting") {
    int actions = 0;
    Menu menu({{.label = "RESET", .action = [&] { ++actions; }}});
    Input click =
        click_in_debug_menu(menu.debug, {DebugMenu::panel_width / 2, DebugMenu::heading_height / 2});
    CHECK(menu.debug.update(click, menu_viewport, menu.entries));
    click = click_in_debug_menu(menu.debug, {DebugMenu::panel_width / 2, DebugMenu::height(1) - 1});
    CHECK(menu.debug.update(click, menu_viewport, menu.entries));
    click = click_in_debug_menu(menu.debug, {DebugMenu::panel_width / 2, DebugMenu::height(1) + 1});
    CHECK_FALSE(menu.debug.update(click, menu_viewport, menu.entries));
    CHECK(actions == 0);
}

TEST_CASE("a hidden debug menu captures nothing") {
    int actions = 0;
    Menu menu({{.label = "RESET", .action = [&] { ++actions; }}});
    menu.debug.visible = false;
    CHECK_FALSE(menu.click(DebugMenu::panel_width / 2));
    CHECK(actions == 0);
}

TEST_CASE("a debug menu stepper runs the button under the click") {
    int actions = 0, value = 0;
    Menu menu({{.label = "SPEED",
                .action = [&] { ++actions; },
                .decrease = [&] { --value; },
                .increase = [&] { ++value; }}});
    const float decrease_middle = DebugMenu::decrease_left + DebugMenu::stepper_width / 2;
    const float increase_middle = DebugMenu::increase_left + DebugMenu::stepper_width / 2;
    CHECK(menu.click(increase_middle));
    CHECK(value == 1);
    menu.click(decrease_middle);
    menu.click(decrease_middle);
    CHECK(value == -1);
    CHECK_MESSAGE(actions == 0, "a button click does not also run the row");
    // The gap between the buttons is split between them, and the [+] zone runs to the edge.
    const float gap_left = DebugMenu::decrease_left + DebugMenu::stepper_width;
    menu.click(DebugMenu::increase_left - 0.5f);
    CHECK(value == 0);
    menu.click(gap_left + 0.5f);
    CHECK(value == -1);
    menu.click(DebugMenu::panel_width - 0.5f);
    CHECK(value == 0);
    CHECK(actions == 0);
}

TEST_CASE("a click beside the stepper buttons still runs the row") {
    bool enabled = false;
    int steps = 0;
    auto row = debug_check("WORKLOAD", enabled);
    row.increase = [&] { ++steps; };
    Menu menu({row});
    menu.click(DebugMenu::label_left(1));
    CHECK(enabled);
    CHECK(steps == 0);
    // Without a [-] button its zone is part of the row.
    menu.click(DebugMenu::decrease_left + 1);
    CHECK_FALSE(enabled);
    CHECK(steps == 0);
}

TEST_CASE("a row without stepper buttons takes clicks across its full width") {
    int actions = 0;
    Menu menu({{.label = "RESET", .action = [&] { ++actions; }}});
    for (const float x :
         {0.5f, DebugMenu::decrease_left + 1, DebugMenu::increase_left + 1, DebugMenu::panel_width - 0.5f})
        CHECK(menu.click(x));
    CHECK(actions == 4);
}

TEST_CASE("a row without an action captures the click and does nothing") {
    float speed = 2;
    Menu menu({debug_number("SPEED", speed, {1, 4, 1})});
    CHECK(menu.click(DebugMenu::label_left(1)));
    CHECK(speed == 2);
}

TEST_CASE("the debug menu scale shrinks with a low or narrow window and is at most one and a half") {
    CHECK(DebugMenu::scale({1280, 800}) == 1.5f);
    CHECK(DebugMenu::scale({3840, 2160}) == 1.5f);
    CHECK(DebugMenu::scale({960, 540}) == doctest::Approx(540 / 420.f));
    CHECK(DebugMenu::scale({240, 800}) == doctest::Approx(240 / 300.f));
    // Twenty-two rows fit a 540-pixel window below the toolbar.
    CHECK(32 + DebugMenu::height(22) * DebugMenu::scale({960, 540}) <= 540);
}
