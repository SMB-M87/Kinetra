# Runtime and launcher implementation

This directory implements the optional SDL2/OpenGL runtime and the generic launcher. Public interfaces live in [include/kinetra](../../include/kinetra/README.md).

## Table of contents

- [Source map](#source-map)
- [Relationships](#relationships)

## Source map

| File | Responsibility | Target |
| --- | --- | --- |
| [runtime.cpp](runtime.cpp) | Window and input lifecycle, backend resources and primitive rendering | `Kinetra::Runtime` |
| [session.cpp](session.cpp) | Direct session event and rendering loop | `Kinetra::Runtime` |
| [launcher.cpp](launcher.cpp) | Menu selection/layout, session hosting and launcher event loop | `Kinetra::Launcher` |

## Relationships

Launcher links Runtime; Runtime does not link Simulation. SDL/OpenGL implementation details stay behind public engine interfaces. Application registration and the executable belong to Kinetra-Lab.

See the [runtime contract](../../docs/engine/runtime.md), [launcher contract](../../docs/engine/launcher-overlays.md) and [test guide](../../tests/README.md). Return to the [source map](../README.md).
