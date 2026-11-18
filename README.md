# Kinetra

Reusable C++20 2D simulation library with an optional SDL2/OpenGL 3.3 runtime.
[Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab) owns applications, the bundled launcher executable and experiments. Kinetra builds independently of Lab.

## Table of contents

- [Features](#features)
- [Quick start](#quick-start)
- [Using Kinetra](#using-kinetra)
- [Working with Kinetra-Lab](#working-with-kinetra-lab)
- [Documentation](#documentation)
- [Platform support](#platform-support)
- [License](#license)

[Documentation index](docs/README.md)

Directory guides: [docs](docs/README.md), [include](include/README.md), [src](src/README.md), [tests](tests/README.md), [tools](tools/README.md).

## Features

- Headless geometry, collisions, physics, spatial queries, navigation and steering.
- Optional window, input, camera, colored primitives, bitmap text and direct sessions.
- Public C++ headers under `include/kinetra/`; CMake targets `Kinetra::Simulation`, optional `Kinetra::Runtime` and `Kinetra::Launcher`.
- Optional reusable launcher menu and session host in `<kinetra/launcher.h>`; application registration and gameplay remain in Lab.

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

Runtime consumers enable `KINETRA_BUILD_RUNTIME` and link `Kinetra::Runtime`. Kinetra also supplies `Kinetra::Launcher` for the generic menu, session host and launcher loop. Integration uses source targets; there is no installed CMake package.

## Working with Kinetra-Lab

Kinetra-Lab includes Kinetra as a Git submodule at `external/Kinetra` and builds it from source. Lab records an exact engine commit. The standalone Kinetra checkout and Lab's submodule are independent Git checkouts; changes do not synchronize automatically.

You can develop engine changes either here or inside Lab's submodule. Build and test the engine with the commands above, then verify the applications in Lab. Publish the Kinetra change first; next select that published commit in Lab's submodule, rebuild/test Lab, and commit and publish Lab's updated submodule pointer. Application-only changes belong in Lab.

For unpublished work in sibling directories, run from Kinetra-Lab:

```sh
cmake -S . -B build-local-core -DCMAKE_BUILD_TYPE=Release -DKINETRA_SOURCE_DIR=../Kinetra
cmake --build build-local-core -j2
ctest --test-dir build-local-core --output-on-failure
```

This local source override does not change Lab's recorded dependency. Before publishing Lab, test against the published commit in `external/Kinetra` using its normal build directory. See the [complete Lab workflow](https://github.com/SMB-M87/Kinetra-Lab#kinetra-development-workflow) for branch handling, engine and Lab publishing order, receiving updates, and build commands.

## Documentation

Directory guides: [public headers](include/README.md), [implementation](src/README.md) and [tests](tests/README.md).

Start with the [documentation index](docs/README.md), [architecture](docs/architecture.md), or [runtime contract](docs/engine/runtime.md).

Contributors follow the [development workflow](docs/workflow.md): every change gets a [design document](docs/design/README.md), tests, a review and documentation, and is verified at the [level its impact requires](docs/testing.md#verification-levels). [AGENTS.md](AGENTS.md) records the rules for agents and developers; [conventions](docs/conventions.md) and the [documentation guide](docs/documentation.md) give the details.

## Platform support

Linux is supported; Artix/Linux is the desktop target. Windows/macOS and 3D are unsupported.
Physics is discrete and fast bodies can tunnel. Navigation does not guarantee arrival in moving crowds.
Rendering has no texture system or pose interpolation. Cross-machine bitwise determinism is not promised.

## License

MIT; see [LICENSE](LICENSE). Generated GLAD retains embedded licenses.
