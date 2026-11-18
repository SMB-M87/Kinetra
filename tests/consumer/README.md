# External consumer fixture

This standalone CMake project verifies that applications can consume Kinetra through public headers and targets using source integration.

## Table of contents

- [File map](#file-map)
- [Build boundary](#build-boundary)

[Parent guide](../README.md) · [Project overview](../../README.md) · [Documentation index](../../docs/README.md)

## File map

| File | Responsibility |
| --- | --- |
| [CMakeLists.txt](CMakeLists.txt) | Add the engine from `KINETRA_SOURCE`, disable its tests and link public targets |
| [main.cpp](main.cpp) | Exercise public simulation APIs and optionally runtime/launcher interfaces |

## Build boundary

The default fixture links `Kinetra::Simulation` and exercises compiled traffic admission and eligibility overloads as well as geometry, navigation and avoidance. `CONSUMER_GRAPHICS=ON` enables the runtime build and links `Kinetra::Launcher`, which exposes its Runtime dependency. The fixture does not open a window and does not replace graphics tests.

See [external consumer commands](../../docs/development.md#external-consumers), the [public API map](../../include/kinetra/README.md) and the [test guide](../README.md).
