// Window-free tests for the launcher library (include/kinetra/runtime/launcher.h): menu paging
// and configuration, the error panel over the menu (error_view.h), SessionHost transitions, and
// debug-menu actions, stepper buttons and typed numbers. The loops that need a window are
// covered by runtime_tests.cpp.
#include <kinetra/runtime/debug_view.h>
#include <kinetra/runtime/error_view.h>
#include <kinetra/runtime/hud_text.h>
#include <kinetra/runtime/launcher.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
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

TEST_CASE("show_configurations opens an application's page with its configuration selected") {
    LaunchMenu menu({{"PLAIN", "NO CONFIGURATIONS", {}, 0}, {"SIZED", "TWO SIZES", {"SMALL", "LARGE"}, 0}});
    const Vec2 viewport{800, 600};
    menu.show_configurations({1, 1});
    CHECK(menu.configuring());
    CHECK(menu.description() == "TWO SIZES");
    CHECK(menu.row_name(menu.selected()) == "LARGE");
    Input confirm;
    confirm.pressed[std::size_t(Key::enter)] = true;
    const auto again = menu.update(confirm, viewport);
    REQUIRE_MESSAGE(again, "Enter on the page did not start the configuration");
    CHECK((again->application == 1 && again->configuration == 1));
    // A pointer that the session left over another row keeps the selection until it moves.
    menu.show_configurations({1, 1});
    Input resting;
    resting.mouse = menu.layout(viewport).row_center(0);
    CHECK_FALSE(menu.update(resting, viewport));
    CHECK_MESSAGE(menu.row_name(menu.selected()) == "LARGE", "a resting pointer replaced the selection");
    resting.mouse.x += 1;
    menu.update(resting, viewport);
    CHECK_MESSAGE(menu.row_name(menu.selected()) == "SMALL", "a moving pointer no longer hovers");
    Input back;
    back.pressed[std::size_t(Key::m)] = true;
    menu.show_configurations({1, 0});
    menu.update(back, viewport);
    CHECK_MESSAGE((!menu.configuring() && menu.selected() == 1), "M leaves the page for the list");
    menu.show_configurations({0, 0});
    CHECK_MESSAGE((!menu.configuring() && menu.selected() == 0),
                  "an application without configurations shows the list");
    menu.show_configurations({1, 1});
    CHECK_THROWS_AS(menu.show_configurations({2, 0}), std::out_of_range);
    CHECK_THROWS_AS(menu.show_configurations({1, 2}), std::out_of_range);
    CHECK_THROWS_AS(menu.show_configurations({0, 1}), std::out_of_range);
    CHECK_MESSAGE((menu.configuring() && menu.row_name(menu.selected()) == "LARGE"),
                  "a rejected selection changed the menu");
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

TEST_CASE("the error panel grows with its message up to the whole window") {
    const Vec2 viewport{1280, 720};
    CHECK(error_panel::lines("", viewport).empty());
    const auto none = error_panel::bounds("", viewport);
    CHECK(none.min.x == none.max.x);
    CHECK_FALSE(error_panel::update(Input{}, "", viewport).captured);

    const auto short_panel = error_panel::bounds("BROKEN.JSON: UNEXPECTED END", viewport);
    CHECK(error_panel::lines("BROKEN.JSON: UNEXPECTED END", viewport).size() == 1);
    CHECK(short_panel.min.x == hud::margin);
    CHECK(short_panel.max.x == viewport.x - hud::margin);
    CHECK_MESSAGE(short_panel.max.y == viewport.y - hud::margin, "the panel sits at the bottom");
    CHECK(short_panel.max.y - short_panel.min.y < 100);

    std::string longer;
    for (int i = 0; i < 40; ++i)
        longer += "/A-RATHER-LONG-DIRECTORY ";
    const auto lines = error_panel::lines(longer, viewport);
    CHECK(lines.size() > 1);
    CHECK(lines.back().find(hud::cut_mark) == std::string::npos);
    CHECK(error_panel::bounds(longer, viewport).min.y < short_panel.min.y);

    std::string huge;
    for (int i = 0; i < 400; ++i)
        huge += "/A-RATHER-LONG-DIRECTORY ";
    const auto all = error_panel::lines(huge, viewport);
    REQUIRE_FALSE(all.empty());
    CHECK_MESSAGE(error_panel::visible_lines(huge, viewport) < all.size(),
                  "a message longer than the window shows part of its lines");
    CHECK(error_panel::visible_lines(longer, viewport) == lines.size());
    const auto whole = error_panel::bounds(huge, viewport);
    CHECK_MESSAGE((whole.min.y >= hud::margin && whole.min.y < hud::margin + hud::line_height),
                  "the panel may use the whole window, less its margins");

    CHECK_MESSAGE(error_panel::lines("TEXT", {20, 720}).empty(),
                  "a window narrower than a character has no panel");
    CHECK_MESSAGE(error_panel::lines("TEXT", {1280, 50}).empty(), "a window lower than a line has no panel");
}

TEST_CASE("the wheel scrolls an error longer than a small window to its end") {
    const Vec2 viewport{320, 240};
    // A long path before the reason, as the blueprint loader writes it.
    std::string message;
    for (int i = 0; i < 12; ++i)
        message += "/A-RATHER-LONG-DIRECTORY";
    message += "/BROKEN.JSON: PARSE ERROR AT LINE 1, COLUMN 11";
    const auto all = error_panel::lines(message, viewport);
    const auto visible = error_panel::visible_lines(message, viewport);
    REQUIRE(visible < all.size());
    REQUIRE(all.back().find("COLUMN 11") != std::string::npos);
    const auto panel = error_panel::bounds(message, viewport);
    Input wheel;
    wheel.mouse = (panel.min + panel.max) / 2;
    wheel.wheel = -100;
    const auto end = error_panel::update(wheel, message, viewport, {}, 0);
    CHECK_MESSAGE(end.first_line == static_cast<double>(all.size() - visible),
                  "scrolling stops at the last line");
    wheel.wheel = 1;
    CHECK(error_panel::update(wheel, message, viewport, {}, end.first_line).first_line == end.first_line - 1);
    wheel.wheel = 100;
    CHECK_MESSAGE(error_panel::update(wheel, message, viewport, {}, 3).first_line == 0, "nor past the first");
    wheel.mouse = {-5, -5};
    wheel.wheel = -1;
    CHECK_MESSAGE(error_panel::update(wheel, message, viewport, {}, 0).first_line == 0,
                  "the wheel off the panel scrolls nothing");

    LaunchMenu menu({{"ONE", "", {}, 0}});
    Input on_panel;
    on_panel.mouse = (panel.min + panel.max) / 2;
    on_panel.wheel = -2;
    menu.update(on_panel, viewport, message);
    CHECK(menu.error_first_line() == 2);
    on_panel.wheel = 0;
    menu.update(on_panel, viewport, "ANOTHER ERROR");
    CHECK_MESSAGE(menu.error_first_line() == 0, "a new message starts at its first line");

    // A touchpad sends fractions of a notch every frame; they add up to whole lines.
    double first = 0;
    Input touch;
    touch.mouse = (panel.min + panel.max) / 2;
    touch.wheel = -0.25f;
    for (int frame = 0; frame < 4; ++frame)
        first = error_panel::update(touch, message, viewport, {}, first).first_line;
    CHECK_MESSAGE(first == doctest::Approx(1), "four quarter notches scroll one line");
}

TEST_CASE("the error panel moves above a region it would cover when its message fits there") {
    const Vec2 viewport{1280, 720};
    const Aabb buttons{{400, 650}, {800, 690}};
    const std::string message = "BROKEN.JSON: UNEXPECTED END";
    const auto above = error_panel::bounds(message, viewport, buttons);
    CHECK_MESSAGE(above.max.y <= buttons.min.y - hud::margin, "a short message ends above the region");
    CHECK(error_panel::lines(message, viewport, buttons).size() == 1);

    const Aabb high{{400, 100}, {800, 140}};
    CHECK_MESSAGE(error_panel::bounds(message, viewport, high).max.y == viewport.y - hud::margin,
                  "a region the panel misses at the bottom leaves it there");

    std::string huge;
    for (int i = 0; i < 400; ++i)
        huge += "/A-RATHER-LONG-DIRECTORY ";
    const auto whole = error_panel::bounds(huge, viewport, buttons);
    CHECK_MESSAGE(whole.max.y == viewport.y - hud::margin,
                  "a message too long for the space above covers it");
    CHECK(error_panel::lines(huge, viewport, buttons) == error_panel::lines(huge, viewport));
}

TEST_CASE("an error panel over the menu takes the pointer and its close button reports the click") {
    LaunchMenu menu({{"ONE", "", {}, 0}, {"TWO", "", {}, 0}});
    const Vec2 viewport{1280, 720};
    const auto layout = menu.layout(viewport);
    const auto exit = layout.footer_center(*menu.footer_x(FooterButton::exit));

    // At 640 x 480 a short error's panel at the bottom would cover EXIT; it ends above the
    // footer instead, so EXIT stays clickable.
    const Vec2 small{640, 480};
    const std::string_view short_error = "BROKEN.JSON: UNEXPECTED END";
    // The lowest row of EXIT's clickable area, which a panel at the bottom would cover.
    const auto small_layout = menu.layout(small);
    const Vec2 small_exit{small_layout.footer_center(*menu.footer_x(FooterButton::exit)).x,
                          small_layout.footer_bounds().max.y - 1};
    REQUIRE(small_layout.footer_contains(small_exit, *menu.footer_x(FooterButton::exit)));
    REQUIRE(error_panel::bounds(short_error, small).min.y <= small_exit.y);
    Input click;
    click.clicked = true;
    click.mouse = small_exit;
    menu.update(click, small, short_error);
    CHECK_MESSAGE(menu.exit_requested(), "a short error leaves EXIT free");

    LaunchMenu covered({{"ONE", "", {}, 0}, {"TWO", "", {}, 0}});
    // Too long for the space above the footer, so its panel covers EXIT.
    std::string error;
    for (int i = 0; i < 600; ++i)
        error += "BROKEN ";
    const auto panel = error_panel::bounds(error, viewport, layout.footer_bounds());
    REQUIRE(panel.min.y < exit.y);
    CHECK_FALSE(covered.update(click, viewport, error));
    CHECK_MESSAGE(!covered.exit_requested(), "a click on the panel does not reach EXIT below it");
    CHECK_FALSE(covered.error_closed());

    const auto close = error_panel::close_bounds(error, viewport, layout.footer_bounds());
    click.mouse = (close.min + close.max) / 2;
    CHECK_FALSE(covered.update(click, viewport, error));
    CHECK(covered.error_closed());
    CHECK_FALSE(covered.exit_requested());

    Input keys;
    keys.mouse = click.mouse;
    keys.pressed[std::size_t(Key::down)] = true;
    covered.update(keys, viewport, error);
    CHECK_MESSAGE(covered.selected() == 1, "keys still reach the menu");
    CHECK_FALSE(covered.error_closed());

    click.mouse = exit;
    covered.update(click, viewport);
    CHECK_MESSAGE(covered.exit_requested(), "without an error EXIT takes the click");
}

TEST_CASE("the error panel coming or going is no pointer move, so keyboard navigation holds") {
    std::vector<LaunchChoice> rows;
    for (int i = 0; i < 6; ++i)
        rows.push_back({"ROW " + std::to_string(i), "", {}, 0});
    LaunchMenu menu(rows);
    const Vec2 viewport{1280, 720};
    // A message too long for the space above the footer: its panel covers every row.
    std::string error;
    for (int i = 0; i < 600; ++i)
        error += "BROKEN ";
    const auto resting = menu.layout(viewport).row_center(3);
    Input still;
    still.mouse = resting;
    REQUIRE(error_panel::update(still, error, viewport, menu.layout(viewport).footer_bounds()).captured);
    menu.update(still, viewport, error);
    CHECK_MESSAGE(menu.selected() == 0, "the row below the panel is not hovered");
    Input down = still;
    down.pressed[std::size_t(Key::down)] = true;
    menu.update(down, viewport, error);
    REQUIRE(menu.selected() == 1);
    menu.update(still, viewport);
    CHECK_MESSAGE(menu.selected() == 1, "the panel going away leaves the keyboard's row");
    Input moved = still;
    moved.mouse.y += 1;
    menu.update(moved, viewport);
    CHECK_MESSAGE(menu.selected() == 3, "a real move hovers again");
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

TEST_CASE("the configuration page has a LAUNCHER button that returns to the application list") {
    const Vec2 viewport{800, 600};
    LaunchMenu menu({{"PLAIN", "NO CONFIGURATIONS", {}, 0}, {"SIZED", "TWO SIZES", {"SMALL", "LARGE"}, 0}});
    CHECK_FALSE(menu.footer_x(FooterButton::launcher));
    CHECK(menu.footer_x(FooterButton::exit) == 150);
    menu.show_configurations({1, 1});
    REQUIRE(menu.footer_x(FooterButton::launcher));
    CHECK(*menu.footer_x(FooterButton::launcher) == 100);
    CHECK(menu.footer_x(FooterButton::exit) == 200);
    CHECK_FALSE(menu.footer_x(FooterButton::previous));
    Input click;
    click.clicked = true;
    INFO("the old EXIT position between the two buttons is no button");
    click.mouse = menu.layout(viewport).footer_center(150);
    CHECK_FALSE(menu.update(click, viewport));
    CHECK((menu.configuring() && !menu.exit_requested()));
    click.mouse = menu.layout(viewport).footer_center(*menu.footer_x(FooterButton::launcher));
    CHECK_FALSE(menu.update(click, viewport));
    CHECK_MESSAGE((!menu.configuring() && menu.selected() == 1), "LAUNCHER leaves the page for the list");
    CHECK_FALSE_MESSAGE(menu.launcher_requested(), "the launcher's own menu handles LAUNCHER itself");
    menu.show_configurations({1, 0});
    click.mouse = menu.layout(viewport).footer_center(*menu.footer_x(FooterButton::exit));
    CHECK_FALSE(menu.update(click, viewport));
    CHECK(menu.exit_requested());
}

TEST_CASE("a nested menu shows LAUNCHER on every page and reports it") {
    for (auto viewport : {Vec2{1280, 800}, Vec2{160, 120}}) {
        CAPTURE(viewport.x);
        std::vector<LaunchChoice> entries{{"SHOWCASE", "", {"ONE", "TWO"}, 0}};
        for (int i = 0; i < 8; ++i)
            entries.push_back({"FLOOR " + std::to_string(i), "", {}, 0});
        LaunchMenu menu(entries, "CATALOG", LauncherButton::every_page);
        INFO("with paging, four buttons a quarter of the footer apart");
        CHECK(menu.footer_x(FooterButton::previous) == 37.5f);
        CHECK(menu.footer_x(FooterButton::launcher) == 112.5f);
        CHECK(menu.footer_x(FooterButton::exit) == 187.5f);
        CHECK(menu.footer_x(FooterButton::next) == 262.5f);
        const auto layout = menu.layout(viewport);
        for (const auto &[left, right] : {std::pair{FooterButton::previous, FooterButton::launcher},
                                          std::pair{FooterButton::launcher, FooterButton::exit},
                                          std::pair{FooterButton::exit, FooterButton::next}}) {
            const Vec2 between = layout.footer_center((*menu.footer_x(left) + *menu.footer_x(right)) / 2);
            CHECK_FALSE(layout.footer_contains(between, *menu.footer_x(left)));
            CHECK_FALSE(layout.footer_contains(between, *menu.footer_x(right)));
        }
        Input click;
        click.clicked = true;
        click.mouse = layout.footer_center(*menu.footer_x(FooterButton::next));
        CHECK_FALSE(menu.update(click, viewport));
        CHECK_MESSAGE(menu.selected() == 6, "NEXT moved to its new place");
        click.mouse = layout.footer_center(*menu.footer_x(FooterButton::previous));
        CHECK_FALSE(menu.update(click, viewport));
        CHECK(menu.selected() == 0);
        INFO("on the configuration page, without paging, LAUNCHER and EXIT share the footer");
        click.mouse = layout.row_center(0);
        CHECK_FALSE(menu.update(click, viewport));
        REQUIRE(menu.configuring());
        CHECK(menu.footer_x(FooterButton::launcher) == 100);
        click.mouse = menu.layout(viewport).footer_center(100);
        CHECK_FALSE(menu.update(click, viewport));
        CHECK_MESSAGE(menu.launcher_requested(), "a nested menu reports LAUNCHER");
        CHECK_MESSAGE(menu.configuring(), "a nested menu leaves the page to its owner");
        CHECK_FALSE(menu.exit_requested());
    }
    LaunchMenu list({{"ONE", "", {}, 0}}, "CATALOG", LauncherButton::every_page);
    Input click;
    click.clicked = true;
    click.mouse = list.layout({800, 600}).footer_center(*list.footer_x(FooterButton::launcher));
    CHECK_FALSE(list.update(click, {800, 600}));
    CHECK_MESSAGE(list.launcher_requested(), "LAUNCHER on the list of a nested menu");
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
    CHECK_FALSE(host.selection());
    host.start({0, 0}, camera);
    auto *session = static_cast<TestSession *>(host.session());
    REQUIRE(session);
    REQUIRE(host.selection());
    CHECK((host.selection()->application == 0 && host.selection()->configuration == 0));
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
    CHECK_MESSAGE(host.selection(), "failed transition forgot the running selection");
    host.return_to_menu();
    CHECK_MESSAGE((!host.session() && destroyed == 1), "return did not release session");
    CHECK_MESSAGE(!host.selection(), "the menu still reports a running selection");
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

namespace {
/// A menu with a row that takes a typed number, `typed`, below a row without one, and the
/// numbers it received.
struct TypingMenu {
    DebugMenu debug;
    std::vector<std::uint64_t> set;
    std::vector<DebugEntry> entries;
    TypingMenu() {
        debug.visible = true;
        entries = {{.label = "PLAIN", .value = [] { return std::string("1"); }},
                   {.label = "TYPED",
                    .value = [] { return std::string("200"); },
                    .set = [this](std::uint64_t value) { set.push_back(value); }}};
    }
    /// One frame with the pointer on `row`, typing `text` and pressing `keys`; returns
    /// whether the menu took the keyboard.
    bool frame(std::size_t row, std::string text, std::initializer_list<Key> keys = {}) {
        Input input;
        input.mouse = debug.origin(menu_viewport) +
                      Vec2{DebugMenu::panel_width / 2, row_middle(row)} * DebugMenu::scale(menu_viewport);
        input.text = std::move(text);
        for (const Key key : keys)
            input.pressed[static_cast<std::size_t>(key)] = true;
        debug.update(input, menu_viewport, entries);
        return debug.keys_captured();
    }
    /// One frame with the pointer outside the menu and no keys.
    bool away() {
        Input input;
        input.mouse = {1, 1};
        debug.update(input, menu_viewport, entries);
        return debug.keys_captured();
    }
};
} // namespace

TEST_CASE("digits typed over a row with set apply on Enter") {
    TypingMenu menu;
    CHECK(menu.frame(1, "7"));
    CHECK(menu.debug.typing());
    CHECK(menu.frame(1, "50"));
    CHECK(menu.set.empty());
    CHECK_MESSAGE(menu.frame(1, "", {Key::enter}), "the Enter that applies belongs to the menu");
    CHECK(menu.set == std::vector<std::uint64_t>{750});
    CHECK_FALSE(menu.debug.typing());
    CHECK_FALSE(menu.frame(1, ""));
}

TEST_CASE("digits and Enter in one frame apply at once and take the keyboard") {
    TypingMenu menu;
    CHECK(menu.frame(1, "1000000", {Key::enter}));
    CHECK(menu.set == std::vector<std::uint64_t>{1000000});
}

TEST_CASE("Backspace removes the last typed digit, and the last one ends typing") {
    TypingMenu menu;
    menu.frame(1, "123");
    CHECK(menu.frame(1, "", {Key::backspace}));
    menu.frame(1, "", {Key::enter});
    CHECK(menu.set == std::vector<std::uint64_t>{12});
    menu.frame(1, "9");
    CHECK(menu.frame(1, "", {Key::backspace}));
    CHECK_FALSE(menu.debug.typing());
    CHECK_FALSE(menu.frame(1, "", {Key::enter}));
    CHECK(menu.set.size() == 1);
}

TEST_CASE("a typed number is discarded by Escape, by leaving the row and by hiding the menu") {
    TypingMenu menu;
    menu.frame(1, "42");
    CHECK_MESSAGE(menu.frame(1, "", {Key::escape}), "the Escape that discards belongs to the menu");
    CHECK_FALSE(menu.debug.typing());

    menu.frame(1, "42");
    CHECK(menu.away());
    CHECK_FALSE(menu.debug.typing());
    CHECK_FALSE(menu.frame(1, "", {Key::enter}));

    menu.frame(1, "42");
    CHECK(menu.frame(0, ""));
    CHECK_FALSE(menu.debug.typing());

    menu.frame(1, "42");
    menu.debug.discard_typing();
    CHECK_FALSE(menu.debug.typing());
    CHECK_FALSE(menu.debug.keys_captured());
    CHECK(menu.set.empty());
}

TEST_CASE("a typed number belongs to its row, not to the index the row had") {
    TypingMenu menu;
    menu.frame(1, "5");
    // An opened section moved another row under the pointer.
    menu.entries.insert(menu.entries.begin() + 1,
                        {.label = "OTHER", .set = [&](std::uint64_t) { FAIL("the wrong row was set"); }});
    menu.frame(1, "", {Key::enter});
    CHECK_FALSE(menu.debug.typing());
    CHECK(menu.set.empty());
}

TEST_CASE("digits over a row without set, or outside the menu, are left to the application") {
    TypingMenu menu;
    CHECK_FALSE(menu.frame(0, "12", {Key::enter}));
    CHECK_FALSE(menu.debug.typing());
    menu.debug.visible = false;
    CHECK_FALSE(menu.frame(1, "12"));
    CHECK_FALSE(menu.debug.typing());
}

TEST_CASE("a typed number keeps at most seven digits and ignores other characters") {
    TypingMenu menu;
    menu.frame(1, "12a3456789");
    menu.frame(1, "", {Key::enter});
    CHECK(menu.set == std::vector<std::uint64_t>{1234567});
}

TEST_CASE("the debug menu scale shrinks with a low or narrow window and is at most one and a half") {
    CHECK(DebugMenu::scale({1280, 800}) == 1.5f);
    CHECK(DebugMenu::scale({3840, 2160}) == 1.5f);
    CHECK(DebugMenu::scale({960, 540}) == doctest::Approx(540 / 420.f));
    CHECK(DebugMenu::scale({240, 800}) == doctest::Approx(240 / 300.f));
    // Twenty-two rows fit a 540-pixel window below the toolbar.
    CHECK(32 + DebugMenu::height(22) * DebugMenu::scale({960, 540}) <= 540);
}
