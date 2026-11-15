#include <kinetra/camera.h>
#include <kinetra/navigation.h>
#include <kinetra/timing.h>

#ifdef CONSUMER_GRAPHICS
#include <kinetra/launcher.h>
#endif

int main() {
#ifdef CONSUMER_GRAPHICS
    kinetra::LaunchMenu menu({{"EXTERNAL", "PUBLIC API", {}, 0}});
    kinetra::Input input;
    input.pressed[std::size_t(kinetra::Key::enter)] = true;
    if (!menu.update(input, {800, 600}))
        return 2;
#endif
    kinetra::World world;
    world.add({});
    world.step(1.f / 120);
    kinetra::Navigation navigation;
    navigation.rebuild({{-2, -2}, {2, 2}}, {});
    return navigation.path({}, {1, 1}).empty() ? 1 : 0;
}
