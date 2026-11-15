#include <kinetra/camera.h>
#include <kinetra/navigation.h>
#include <kinetra/timing.h>

#ifdef CONSUMER_GRAPHICS
#include <kinetra/session.h>
// Retain a relocation to a compiled Runtime entry point without opening a window.
auto consumer_run_session = &kinetra::run_session;
#endif

int main() {
#ifdef CONSUMER_GRAPHICS
    // Public session and input types remain independent of Lab.
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
