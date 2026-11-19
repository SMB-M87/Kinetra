# Architecture

The public API is under `include/kinetra/`. Three core CMake targets express linking boundaries; directories are not separate libraries.

## Table of contents

- [System context](#system-context)
- [Components and build dependencies](#components-and-build-dependencies)
- [Object ownership](#object-ownership)
- [Frame interaction](#frame-interaction)
- [Responsibility map](#responsibility-map)

[Project overview](../README.md) · [Documentation index](README.md)

## System context

```mermaid
flowchart LR
    K[Kinetra public API]
    Consumer[External CMake consumer] -->|add_subdirectory| K
    Lab[Kinetra-Lab applications and launcher] --> K
    K --> Sim[Headless simulation]
    K --> Runtime[Optional runtime]
    Runtime --> Backend[SDL2 / OpenGL 3.3 / GLAD]
```

Applications and the bundled launcher executable live in Kinetra-Lab. Kinetra owns the generic `Kinetra::Launcher` menu, host and event loop. Core builds and tests need no sibling repository. Lab consumes the public source targets through `add_subdirectory`.

## Components and build dependencies

```mermaid
flowchart TD
    App[External application] --> Sim[Kinetra::Simulation]
    App --> Runtime[Kinetra::Runtime]
    App --> Launcher[Kinetra::Launcher]
    Launcher --> Runtime
    App --> UI[Header-only bitmap text / debug menu / help panel]
    UI -->|draw calls| Runtime
    Sim --> Headless[Math / physics / spatial grids / navigation / steering / avoidance / traffic]
    Runtime --> Parts[Window, input, batched drawing, run_session, SettingsOverlay]
    Parts --> Platform[SDL2 / OpenGL 3.3 / GLAD, private]
```

Arrows denote consumption; the header-only UI calls Runtime drawing methods rather than adding a linked module, while `SettingsOverlay` is compiled into Runtime. `Camera2D`, `FixedClock` and `RateCounter` are header-only utilities that need no runtime. Runtime does not link Simulation, and engine source never includes application headers. [CMake](../CMakeLists.txt) is authoritative for target composition.

## Object ownership

```mermaid
classDiagram
    Runtime "1" *-- "1" Impl : unique_ptr
    Runtime "1" *-- "1" Camera2D : value
    Impl "1" *-- "1" SDL_Window : destroy on cleanup
    Impl : owns SDL_GLContext
    Session <|-- ApplicationSession
    World "1" *-- "0..*" Body : values
    Navigation "1" *-- "0..*" Body : obstacle snapshot
    DebugOption ..> ApplicationState : borrowed pointer / callback
```

`Impl` is the private `Runtime::Impl`, which keeps SDL and OpenGL types out of the public header. The caller owns Runtime and the concrete Session and keeps both alive throughout `run_session`. World owns body values; Navigation copies obstacle geometry. Overlay callbacks and pointers borrow application state. Only Session uses inheritance here; body shape and motion are enums.

## Frame interaction

```mermaid
sequenceDiagram
    participant Main as run_session
    participant R as Runtime
    participant S as Application session
    participant C as FixedClock
    Main->>R: poll events and update Input
    Main->>S: advance(elapsed, input, camera)
    S->>S: debug input capture before game mapping
    S->>S: commands and camera
    S->>C: advance(elapsed)
    loop consumed ticks while unpaused
        S->>S: application update and physics
    end
    Main->>S: request()
    alt menu or exit
        Main-->>Main: end the run without rendering
    else none
        Main->>S: render(runtime)
        S->>R: begin, world primitives, overlays, end
        R->>R: batch upload, draw, swap buffers
        Main->>R: title(session title)
    end
```

`run_session` supplies wall-clock elapsed time, polls input, reads the session's request after every advance and updates the window title after rendering. A session that asks to leave is never drawn again; `run_launcher` follows the same rule ([sessions and requests](engine/launcher-overlays.md#sessions-and-requests)). Applications own pause behavior and fixed-update policies, including rates and backlog consumption. Detailed contracts: [runtime](engine/runtime.md), [physics](engine/math-physics.md), [navigation](engine/navigation.md).

## Responsibility map

| Engine mechanism | Application policy |
| --- | --- |
| Vector, shape, contact, impulse and grid query | Pixel/world conversion and damage/rewards |
| A*, swept clearance, path shortening and steering | Wander targets, combat paths, retries and timeouts |
| Runtime, input values, camera and fixed clock | Bindings, zoom bounds, pause and update frequencies |
| Direct sessions, generic launcher and shared overlay layout | Lab application registration, factories, difficulty and debug values |
| Primitive drawing and bitmap text | Terrain appearance, health bars and AI visualizations |

[Decision 0008](decisions/0008-public-engine-and-consumers.md) records the public-boundary migration and compatibility costs.
