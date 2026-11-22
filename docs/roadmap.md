# Roadmap

The backlog of Kinetra: engine work that is wanted but not finished, from agreed follow-ups to loose ideas. Work in progress is described in its design document; this page says what comes after it. Application work lives in the [Kinetra-Lab roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md).

## Table of contents

- [How to use this page](#how-to-use-this-page)
- [Now](#now)
- [Next](#next)
- [Later](#later)
- [Ideas](#ideas)

[Documentation index](README.md) · [Project overview](../README.md) · [Design documents](design/README.md)

## How to use this page

Sections, item format and the moves between sections are defined in the [documentation guide](documentation.md#roadmap). The `roadmap` skill adds, shapes and closes items, for example during a brainstorm with the user ([skill](../.agents/skills/roadmap/README.md)). A merged item is removed; its design document is the record.

Within each section the first item comes first. The order was proposed on 2026-11-21 together with the [Kinetra-Lab roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md), which is the plan for the applications, and settled with the user on 2026-11-22 ([decisions](design/2026-11-22-roadmap-decisions.md)); Lab's quality roadmap of 2026-11-18 is closed, and what it left is listed here and there.

## Now

Nothing in progress in Kinetra itself.

## Next

- **Agent inspector overlay** · runtime, with Lab · added 2026-11-21
  A reusable overlay that shows the rows of one selected agent (a title, label–value rows in groups) beside it or in a corner, so the user sees an agent's motion settings, goal and state while it runs. Kinetra owns layout, drawing and input capture, as for every overlay; applications supply the selection and the rows as headless values, as `DebugEntry` does for the debug menu ([debug_view.h](../include/kinetra/runtime/debug_view.h)).
  Next step: design it with Lab's item of the same name, which starts with i4sim.
- **Linux beyond Arch** · build, docs, CI, with Lab · added 2026-11-21
  Kinetra names Artix/Linux its desktop target and documents only `pacman` ([AGENTS.md](../AGENTS.md#identity-and-scope), [development](development.md#dependencies-and-options)). Support Linux in general: dependency commands per distribution family (Arch and Artix, Debian and Ubuntu, Fedora) and a second CI job on Ubuntu LTS (the user's choice of 2026-11-22), while the pinned Arch toolchain stays the reference that CI's warnings and formatting follow. Fedora gets documented commands but no CI job.
  Next step: build and test Kinetra on Ubuntu 24.04 and fix what breaks. Lab's part waits for its golden tests to hold on every toolchain.

## Later

- **Shared passing mechanism** · navigation, with Lab · added 2026-11-21
  Move the proven, application-free part of i4sim's predictive passing (conflict prediction, lane agreement, maneuver guard) into Kinetra, with a design document and an ADR; factory policy (traffic zones, parking, junction rules) stays in i4sim. Decide whether the four overlapping steering models (`steer`, `priority_steer`, `tangent_steer`, `avoid_collisions`, which take different inputs and return acceleration or velocity) can share one agent-motion interface, and move i4sim-shaped defaults, such as the `PrioritySteering` values in millimeters per tick, out of the engine.
  Depends on: Lab's *Stall detection and remedies in i4sim*, which settles whether making way and a temporary motion fallback belong in the shared part.
- **Engine performance** · simulation, with Lab · added 2026-11-21
  Optimize the engine code that Lab's benchmarks spend their time in (spatial queries, steering, weighted-grid search, contact response), one hotspot per pull request, with a benchmark before and after and the golden lines unchanged.
  Depends on: Lab's *Performance baseline and hotspots*.
- **CI image with the toolchain built in** · CI, with Lab · added 2026-11-22
  CI installs the pinned toolchain from the Arch Linux Archive in every run, and falls back to the current packages when the archive does not answer ([CI falls back to current Arch packages](design/2026-11-21-ci-mirror-fallback.md)). An image with the pinned packages already installed, published to GitHub's container registry, would make CI independent of the archive and faster. It can only be built while the archive answers.
  Next step: a design: who builds and publishes the image, and how a snapshot bump updates it.
- **Frame read-back on `Runtime`** · runtime · added 2026-11-21
  A helper that returns the current frame's pixels, so Lab's capture tool and the graphics tests stop calling `glReadPixels` themselves.
  Where: [capture tool open points](https://github.com/SMB-M87/Kinetra-Lab/blob/main/tools/capture/README.md#open-points).
- **Headless timing outside `runtime/`** · include layout, with Lab · added 2026-11-21
  `<kinetra/runtime/timing.h>` (`FixedClock`, `RateCounter`) is header-only and headless, yet sits under `runtime/`, and the Demo, Dotio and i4sim logic include it. Move it to `simulation/` or `view/`.
  Where: [Dotio restructuring, open points](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-21-dotio-restructuring.md#open-points).
- **Raster route target** · navigation · added 2026-11-21
  A `WeightedGrid` counterpart of `route_target`, so agents that follow a grid route cut corners that are already in sight; Dotio's pursuers would move more smoothly.
  Where: [Dotio restructuring, open points](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-21-dotio-restructuring.md#open-points).

## Ideas

Wishes of the user, not yet agreed or shaped. Each needs a design session (`grilling`) before it moves up.

- **Android runtime** · runtime, build, with Lab · added 2026-11-21
  Run the runtime, and so Kinetra-Lab, on Android. SDL2 supports Android, but the runtime asks for an OpenGL 3.3 core context with `#version 330 core` shaders (`src/runtime/runtime.cpp`), which Android does not offer: it needs an OpenGL ES 3.0 path. Touch has to map onto Kinetra's input types, and the build needs the NDK and a Gradle wrapper.
  Open questions: which devices, whether an emulator in CI is enough, and whether the overlays work with touch. Today Kinetra supports Linux, and Lab also builds for Windows; nothing may imply Android support before it is tested.
- **3D rendering** · runtime, view, with Lab · added 2026-11-21
  Kinetra is a 2D engine, by its identity in [AGENTS.md](../AGENTS.md#identity-and-scope) and the history in [ADR 0001](decisions/0001-independent-2d-foundation.md). 3D rendering needs a depth buffer, meshes, a perspective camera and lighting shaders next to the 2D renderer. This changes a boundary, so it needs an ADR.
  Open questions: only 3D rendering of the existing simulation, or 3D math and physics too; a separate module next to 2D or one renderer for both; own math or a library such as glm. Motivated by the Lab idea of a 3D Skatepark lab.
- **Documentation as a site** · docs, with Lab · added 2026-11-21
  The user asked whether the docs could move to the GitHub wiki. A move would lose what the docs get from living next to the code; a site generated from `docs/` (wiki or Pages) could mirror them. Assessment in Lab's idea of the same name.
