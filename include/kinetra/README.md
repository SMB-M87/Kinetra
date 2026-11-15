# Public Kinetra API

Consumers include these headers as `<kinetra/name.h>`. This is the engine's first-party public interface; backend types remain private.

## Table of contents

**Navigation:**

[Parent guide](../README.md) · [Project overview](../../README.md) · [Documentation index](../../docs/README.md)

**On this page:**

- [Header map](#header-map)
- [Implementation and contracts](#implementation-and-contracts)

## Header map

| Headers | Responsibility | Linking |
| --- | --- | --- |
| [math.h](math.h) | Vectors, geometry and numeric helpers | Header-only |
| [physics.h](physics.h), [spatial_grid.h](spatial_grid.h), [collision_grid.h](collision_grid.h) | Bodies, contacts and spatial queries | `Kinetra::Simulation` |
| [navigation.h](navigation.h), [steering.h](steering.h) | Paths, routes and local steering | `Kinetra::Simulation` |
| [engine.h](engine.h), [session.h](session.h) | Runtime, input, drawing and direct sessions | `Kinetra::Runtime` |
| [launcher.h](launcher.h) | Generic menu, session host and launcher loop | `Kinetra::Launcher` |
| [camera.h](camera.h), [timing.h](timing.h) | Coordinate transforms and fixed clock | Header-only |
| [bitmap_text.h](bitmap_text.h), [debug_view.h](debug_view.h), [help_view.h](help_view.h) | Text rendering and shared overlays | Header-only; drawing uses Runtime |

## Implementation and contracts

Compiled implementations live in [simulation](../../src/simulation/README.md) and [engine](../../src/engine/README.md). Runtime and Launcher are optional; simulation APIs remain headless.

Use the [subsystem contracts](../../docs/engine/README.md) for behavior and ownership, and the [external consumer](../../tests/consumer/README.md) for linking examples. Return to the [header guide](../README.md).
