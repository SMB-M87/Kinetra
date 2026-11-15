# Engine implementation

Compiled engine mechanisms are split into three public linking boundaries. The root [CMakeLists.txt](../CMakeLists.txt) defines their source lists.

## Table of contents

- [Source map](#source-map)
- [Boundaries and verification](#boundaries-and-verification)

## Source map

| Source | Target | Responsibility |
| --- | --- | --- |
| [simulation/](simulation/README.md) | `Kinetra::Simulation` | Headless implementation directory |
| [engine/](engine/README.md) | `Kinetra::Runtime`, `Kinetra::Launcher` | Runtime and launcher implementation directory |
| `simulation/physics.cpp` | `Kinetra::Simulation` | Contacts, integration, impulses and World stepping |
| `simulation/spatial_grid.cpp` | `Kinetra::Simulation` | AABB indexing and overlap queries |
| `simulation/collision_grid.cpp` | `Kinetra::Simulation` | Occupancy grid collision and segment queries |
| `simulation/navigation.cpp`, `simulation/steering.cpp` | `Kinetra::Simulation` | Path planning, route following and local steering |
| `engine/runtime.cpp` | `Kinetra::Runtime` | SDL window/input lifecycle and OpenGL rendering |
| `engine/session.cpp` | `Kinetra::Runtime` | Direct session event and rendering loop |
| `engine/launcher.cpp` | `Kinetra::Launcher` | Generic menu, session host and launcher loop |
| `glad.c` | `Kinetra::Runtime` | Generated OpenGL loader; preserve generated contents |

## Boundaries and verification

Simulation depends only on the standard library and remains headless. Runtime uses SDL2/OpenGL and does not link Simulation. Public contracts live in [include/kinetra](../include/README.md); backend details stay private to the implementation. The generic launcher links Runtime. Application registration and the bundled launcher executable belong to Kinetra-Lab.

Core behavior is covered by [simulation tests](../tests/simulation_tests.cpp), [navigation tests](../tests/navigation_tests.cpp) and optional context-backed [runtime tests](../tests/runtime_tests.cpp). See [development](../docs/development.md) for build and test commands, and [architecture](../docs/architecture.md) for ownership and interactions.
