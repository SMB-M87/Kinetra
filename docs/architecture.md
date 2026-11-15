# Architecture

The public API is under `include/kinetra/`. Three CMake targets express linking boundaries; directories are not separate libraries.

## Table of contents

- [System context](#system-context)
- [Components and build dependencies](#components-and-build-dependencies)
- [Object ownership](#object-ownership)
- [Frame interaction](#frame-interaction)
- [Responsibility map](#responsibility-map)

## System context

```mermaid
flowchart LR
    Demo[Demo reference application] --> K[Kinetra public API]
    Dotio[Dotio migration application] --> K
    Consumer[External CMake consumer] -->|add_subdirectory| K
    Lab[Future Kinetra-Lab: Dotio / i4sim] -. planned consumer .-> K
    K --> Sim[Headless simulation]
    K --> Runtime[Optional runtime]
    Runtime --> Backend[SDL2 / OpenGL 3.3 / GLAD]
```

Demo and Dotio are current applications. Kinetra-Lab is a future repository, not a current dependency. No sibling repository is needed.

## Components and build dependencies

```mermaid
flowchart TD
    App[examples/app: registration and CLI] --> Demo[Demo presentation and session]
    App --> Dotio[Dotio presentation and session]
    App --> Launcher[Kinetra::Launcher]
    Launcher --> Runtime[Kinetra::Runtime]
    Demo --> Runtime
    Dotio --> Runtime
    Demo --> DemoPolicy[Demo scene / terrain / agents]
    Dotio --> DotioPolicy[Dotio world / AI / combat / legacy adapters]
    DemoPolicy --> Sim[Kinetra::Simulation]
    DotioPolicy --> Sim
    Sim --> Headless[Math / collision / spatial grid / navigation / steering]
    Demo --> UI[Header-only debug / help / bitmap text]
    Dotio --> UI
    UI --> Runtime
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
    SessionHost "1" *-- "0..1" Session : unique_ptr
    SessionHost "1" *-- "1..*" ApplicationRegistration : values
    Session <|-- DemoSession
    Session <|-- DotioSession
    DemoSession *-- SandboxApplication
    SandboxApplication *-- Sandbox
    Sandbox *-- World
    Sandbox *-- Navigation
    Sandbox *-- Terrain
    Sandbox "1" *-- "0..*" Agent : values
    World "1" *-- "0..*" Body : values
    Agent ..> Body : index until scene compaction
    DotioSession *-- DotioApplication
    DotioApplication *-- LogicWorld : unique_ptr stable address
    class LogicWorld["logic::World"]
    DebugOption ..> ApplicationState : borrowed pointer / callback
```

Runtime outlives sessions; application factories receive a camera value to configure. Scene compaction remaps Demo body indices. Only `Session` uses inheritance here; body shape and motion are enums. Dotio's static raster limits it to one active world, despite the generic host supporting arbitrary factories.

## Frame interaction

```mermaid
sequenceDiagram
    participant Main as run_launcher / run_session
    participant R as Runtime
    participant H as SessionHost (optional)
    participant S as Application session
    participant C as FixedClock
    Main->>R: poll events and update Input
    Main->>H: menu selection / return / advance
    H->>S: advance(elapsed, input, camera)
    Note over H,S: New sessions receive zero time and empty input
    S->>S: debug input capture before game mapping
    S->>S: commands, camera and streaming
    S->>C: advance(elapsed)
    loop consumed ticks while unpaused
        S->>S: application update and physics
    end
    Main->>S: render(runtime)
    S->>R: begin, world primitives, overlays, end
    R->>R: batch upload, draw, swap buffers
```

The standard engine launcher loop supplies wall-clock time; direct consumers can use `run_session`. Applications own pause and fixed-update policies. Demo runs at 120 Hz; Dotio consumes 60 Hz movement ticks with 20 Hz AI. Paused time is consumed without replay on resume. Detailed flows: [runtime](engine/runtime.md), [physics](engine/math-physics.md), [navigation](engine/navigation.md).

## Responsibility map

| Engine mechanism | Application policy |
| --- | --- |
| Vector, shape, contact, impulse and grid query | Pixel/world conversion and damage/rewards |
| A*, swept clearance, path shortening and steering | Wander targets, combat paths, retries and timeouts |
| Runtime, input values, camera and fixed clock | Bindings, zoom bounds, pause and update frequencies |
| Registered menu, session ownership and shared overlay layout | Application factories, difficulty choices and debug values |
| Primitive drawing and bitmap text | Terrain appearance, health bars and AI visualizations |

[Decision 0008](decisions/0008-public-engine-and-consumers.md) records the public-boundary migration and compatibility costs.
