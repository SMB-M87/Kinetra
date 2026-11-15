# 0004: Separate sandbox session policy from input and rendering

Historical decision. The current repository boundary is defined by [0009](0009-core-and-lab.md); application sources, tests and behavior guides now belong to Kinetra-Lab.


## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Alternatives and consequences](#alternatives-and-consequences)
- [Verification](#verification)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

The sandbox frame loop mixed key bindings, pause/reset, camera changes, pointer actions, fixed stepping, primitive drawing and title text. Scene updates depended on runtime `Input` and key enums. Session behavior could only be exercised through the executable, even though the physics and terrain already had independent tests.

## Decision

Use the existing simulation/runtime boundary and extract three focused responsibilities:

- An input Adapter maps held keys, transitions and mouse state into application-owned command values.
- `SandboxApplication` is a small Service Layer coordinating a scene, fixed clock and session settings. It consumes commands and exposes read-only state and an action-result enum. Scene rules continue to own navigation, teleport validation and population behavior.
- A separate renderer draws scene values, routes and help and formats window-title text. `main` is the composition root and supplies wall-clock time and the runtime-owned camera.

`PlayerControl` contains movement direction and rotation intent, without hardware bindings. The scene depends on this value rather than runtime input. Two opposing rotation inputs still cancel navigation even when their net angular velocity is zero.

Preserve the frame order: toggles/reset, camera update, terrain streaming, pointer resolution, follow/restream, clock consumption, unpaused fixed updates, final following, rendering. Consume elapsed time while paused so resuming does not replay it. Reset keeps the seed, pause, gravity, follow and presentation settings while rebuilding scene state and clearing the previous action result.

Build the application in its own target linked only to Simulation. It is example code, not a new public engine API. Build it when the demo or tests are enabled; library-only consumers do not need to build the sandbox.

## Alternatives and consequences

Keeping helper functions in `main` would reduce function size but leave the session workflow tied to runtime input. Abstract renderer interfaces and polymorphic command objects are unnecessary for the current single backend and synchronous input. Value commands provide the needed seam without an execution registry, event bus or undo infrastructure. Persistence repositories, mappers and transaction patterns have no role in this in-memory simulation.

The application still uses the concrete sandbox scene and pure camera values. The renderer reads that scene's existing body/route representation through const access. Presentation reads the scene directly through const access. Scene population storage and the simulation algorithms retain their existing representation; this refactor does not redesign them.

No public simulation/runtime API, input binding, CLI option, world format or persistence behavior changes. Reverting the boundary extraction requires no data migration.

## Verification

[Application tests](https://github.com/SMB-M87/Kinetra-Lab/blob/main/launcher/demo/tests/sandbox_application_tests.cpp) exercise pause/resume, camera/pointer ordering, failed actions, reset, presentation settings and player intent without a window. Existing [terrain tests](https://github.com/SMB-M87/Kinetra-Lab/blob/main/launcher/demo/tests/terrain_tests.cpp) continue to cover scene rules. [Runtime tests](https://github.com/SMB-M87/Kinetra-Lab/blob/main/launcher/tests/runtime_tests.cpp) cover input adaptation, click modifier precedence and complete renderer initialization/drawing alongside graphics and overlay checks. Run both headless and graphics CTest suites and the bounded sandbox smoke run described in the [README](../development.md#checks).
