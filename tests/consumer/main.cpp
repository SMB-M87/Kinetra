#include <kinetra/camera.h>
#include <kinetra/navigation.h>
#include <kinetra/timing.h>

#ifdef CONSUMER_GRAPHICS
#include <kinetra/session.h>
#include <kinetra/launcher.h>
auto consumer_run_launcher = &kinetra::run_launcher;
// Retain a relocation to a compiled Runtime entry point without opening a window.
auto consumer_run_session = &kinetra::run_session;
#endif

int main() {
#ifdef CONSUMER_GRAPHICS
    // Public session and input types remain independent of Lab.
    kinetra::LaunchMenu menu({{"CONSUMER", "External application", {}, 0}});
    if (menu.row_count() != 1)
        return 3;
    kinetra::Input input;
    if (input.held[std::size_t(kinetra::Key::enter)])
        return 2;
#endif
    kinetra::World world;
    world.add({});
    world.step(1.f / 120);
    kinetra::Navigation navigation;
    navigation.rebuild({{-2, -2}, {2, 2}}, {});
    return navigation.path({}, {1, 1}).empty() ? 1 : 0;
}
