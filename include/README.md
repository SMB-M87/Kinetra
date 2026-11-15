# Public headers

Consumer-facing interfaces live in [kinetra/](kinetra/README.md). Include them as `<kinetra/physics.h>`, for example, using the public CMake targets.

## Table of contents

- [API map](#api-map)
- [Dependencies and ownership](#dependencies-and-ownership)

[Parent guide](../README.md) · [Documentation index](../docs/README.md)

Directory guides: [KHR](KHR/README.md), [glad](glad/README.md), [kinetra](kinetra/README.md).

## API map

| Headers in `kinetra/` | Purpose | Contract |
| --- | --- | --- |
| `math.h`, `physics.h`, `spatial_grid.h`, `collision_grid.h` | Geometry, bodies, contacts and spatial queries | [Math and physics](../docs/engine/math-physics.md) |
| `navigation.h`, `steering.h` | Obstacle-aware paths and local steering | [Navigation](../docs/engine/navigation.md) |
| `engine.h`, `session.h` | Runtime, input, drawing and direct sessions | [Runtime](../docs/engine/runtime.md) |
| `launcher.h` | Generic menu, session host and launcher loop (`Kinetra::Launcher`) | [Launcher](../docs/engine/launcher-overlays.md) |
| `camera.h`, `timing.h` | Header-only coordinate transforms and fixed clock | [Runtime](../docs/engine/runtime.md) |
| `bitmap_text.h`, `debug_view.h`, `help_view.h` | Header-only text and shared overlays | [Overlays](../docs/engine/launcher-overlays.md) |

## Dependencies and ownership

Link compiled simulation APIs through `Kinetra::Simulation`; runtime and drawing consumers use the optional `Kinetra::Runtime`. Camera, timing and math utilities are header-only and do not require SDL/OpenGL. Public engine interfaces keep backend implementation types private.

[glad/](glad/README.md) and [KHR/](KHR/README.md) contain generated or third-party backend headers. Preserve their licenses and generated contents; application code should use `kinetra/` interfaces.

See the [implementation map](../src/README.md), [architecture](../docs/architecture.md) and [build instructions](../docs/development.md).
