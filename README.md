# Kinetra

Reusable C++20 2D simulation library with an optional SDL2/OpenGL 3.3 runtime.
[ Kinetra-Lab ](https://github.com/SMB-M87/Kinetra-Lab) owns Launcher, Demo, Dotio and future experiments. Kinetra builds independently of Lab.

## Table of contents

- [Features](#features)
- [Quick start](#quick-start)
- [Using Kinetra](#using-kinetra)
- [Documentation](#documentation)
- [Platform support](#platform-support)
- [License](#license)

## Features

- Headless geometry, collisions, physics, spatial queries, navigation and steering.
- Optional window, input, camera, colored primitives, bitmap text and direct sessions.
- Public C++ headers under `include/kinetra/`; CMake targets `Kinetra::Simulation` and optional `Kinetra::Runtime`.

## Quick start

Requires C++20, CMake 3.16+, a build tool. The default build is headless.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Enable `-DKINETRA_BUILD_RUNTIME=ON` for SDL2/OpenGL (development packages required).
See [development](docs/development.md) for graphics verification. Run applications from Kinetra-Lab.

## Using Kinetra

Source integration:

```cmake
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(external/Kinetra)
add_executable(my_simulation main.cpp)
target_link_libraries(my_simulation PRIVATE Kinetra::Simulation)
```

```cpp
#include <kinetra/physics.h>

int main() {
    kinetra::World world;
    world.add({});
    world.step(1.f / 120);
}
```

Runtime consumers enable `KINETRA_BUILD_RUNTIME` and link `Kinetra::Runtime`. Lab supplies `Kinetra::Launcher`. Integration uses source targets; there is no installed CMake package.

## Documentation

Start with the [documentation index](docs/README.md), [architecture](docs/architecture.md), or [runtime contract](docs/engine/runtime.md). [AGENTS.md](AGENTS.md) records engineering rules.

## Platform support

Linux is supported; Artix/Linux is the desktop target. Windows/macOS and 3D are unsupported.
Physics is discrete and fast bodies can tunnel. Navigation does not guarantee arrival in moving crowds.
Rendering has no texture system or pose interpolation. Cross-machine bitwise determinism is not promised.

## License

MIT; see [LICENSE](LICENSE). Generated GLAD retains embedded licenses.
