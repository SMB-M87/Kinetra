# Architecture

The public API is under `include/kinetra/`. Three core CMake targets express linking boundaries; directories are not separate libraries.

## Table of contents

- [System context](#system-context)
- [Components and build dependencies](#components-and-build-dependencies)
- [Object ownership](#object-ownership)
- [Frame interaction](#frame-interaction)
- [Responsibility map](#responsibility-map)

[Directory guide](README.md) · [Project overview](../README.md) · [Documentation index](README.md)

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
    App[External application] --> Runtime[Kinetra::Runtime]
    App --> Launcher[Kinetra::Launcher]
    Launcher --> Runtime
    App --> Sim[Kinetra::Simulation]
    App --> UI[Header-only debug / help / bitmap text]
    UI --> Runtime
    Sim --> Headless[Math / collision / spatial grid / navigation / steering]
    Runtime --> Platform[SDL input / window and OpenGL batches]
```

Arrows denote consumption; UI headers call Runtime drawing methods rather than adding a linked module. Camera and FixedClock are pure header utilities. Runtime does not link Simulation, and engine source never includes application headers. [CMake](../CMakeLists.txt) is authoritative for target composition.

## Object ownership

```mermaid
classDiagram
    Runtime "1" *-- "1" RuntimeImpl : unique_ptr
    Runtime "1" *-- "1" Camera2D : value
    RuntimeImpl "1" *-- "1" SDL_Window : destroy on cleanup
    RuntimeImpl : owns SDL_GLContext
    Session <|-- ApplicationSession
    World "1" *-- "0..*" Body : values
    Navigation "1" *-- "0..*" Body : obstacle snapshot
    DebugOption ..> ApplicationState : borrowed pointer / callback
```

The caller owns Runtime and the concrete Session and keeps both alive throughout `run_session`. World owns body values; Navigation copies obstacle geometry. Overlay callbacks and pointers borrow application state. Only Session uses inheritance here; body shape and motion are enums.

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
    Main->>S: render(runtime)
    S->>R: begin, world primitives, overlays, end
    R->>R: batch upload, draw, swap buffers
```

`run_session` supplies wall-clock elapsed time, polls input and updates the window title after rendering. Applications own pause behavior and fixed-update policies, including rates and backlog consumption. Detailed contracts: [runtime](engine/runtime.md), [physics](engine/math-physics.md), [navigation](engine/navigation.md).

## Responsibility map

| Engine mechanism | Application policy |
| --- | --- |
| Vector, shape, contact, impulse and grid query | Pixel/world conversion and damage/rewards |
| A*, swept clearance, path shortening and steering | Wander targets, combat paths, retries and timeouts |
| Runtime, input values, camera and fixed clock | Bindings, zoom bounds, pause and update frequencies |
| Direct sessions, generic launcher and shared overlay layout | Lab application registration, factories, difficulty and debug values |
| Primitive drawing and bitmap text | Terrain appearance, health bars and AI visualizations |

[Decision 0008](decisions/0008-public-engine-and-consumers.md) records the public-boundary migration and compatibility costs.
