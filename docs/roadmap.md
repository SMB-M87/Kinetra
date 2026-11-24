# Roadmap

The backlog of Kinetra: engine work that is wanted but not finished, from agreed follow-ups to loose ideas. Work in progress is described in its design document, or for a docs-only change in its pull request; this page says what comes after it. Application work lives in the [Kinetra-Lab roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md).

## Table of contents

- [How to use this page](#how-to-use-this-page)
- [Now](#now)
- [Next](#next)
- [Later](#later)
- [Ideas](#ideas)

[Documentation index](README.md) · [Project overview](../README.md) · [Design documents](design/README.md)

## How to use this page

Sections, item format and the moves between sections are defined in the [documentation guide](documentation.md#roadmap). The `roadmap` skill adds, shapes and closes items, for example during a brainstorm with the user ([skill](../.agents/skills/roadmap/README.md)). A merged item is removed; its design document, or for a docs-only change its pull request, is the record.

Within each section the first item comes first. The order was proposed on 2026-11-21 together with the [Kinetra-Lab roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md), which is the plan for the applications, and settled with the user on 2026-11-22 ([decisions](design/2026-11-22-roadmap-decisions.md)); on 2026-11-23 the user put *Leaner agentic cycle* first, in both roadmaps; the other items of that day stand at the end of their sections ([design](design/2026-11-23-roadmap-media-labs-cycle.md#decisions)). Lab's quality roadmap of 2026-11-18 is closed, and what it left is listed here and there.

## Now

Nothing in progress in Kinetra itself.

## Next

Nothing planned next; the items of *Later* follow.

## Later

- **Linux support follow-ups** · build, docs, CI, with Lab · added 2026-11-22
  One point *Linux beyond Arch* left open: the Fedora command in [development](development.md#dependencies-and-options) is not checked against Fedora's repositories (whether `SDL2-devel` resolves under `sdl2-compat`, whether `mesa-dri-drivers` is needed). The other, whether Ubuntu is a required status, the user settled on 2026-11-23: yes, through the `linux` matrix ([CI Linux matrix](design/2026-11-23-ci-linux-matrix.md)).
  Next step: the user checks the Fedora command, or has a session that can reach Fedora's package hosts check it; a Fedora matrix entry can follow. Where: [design, open points](design/2026-11-22-linux-beyond-arch.md#open-points).

- **Shared passing mechanism** · navigation, with Lab · added 2026-11-21
  Move the proven, application-free part of i4sim's predictive passing (conflict prediction, lane agreement, maneuver guard) into Kinetra, with a design document and an ADR; factory policy (traffic zones, parking, junction rules) stays in i4sim. Decide whether the four overlapping steering models (`steer`, `priority_steer`, `tangent_steer`, `avoid_collisions`, which take different inputs and return acceleration or velocity) can share one agent-motion interface, and move i4sim-shaped defaults, such as the `PrioritySteering` values in millimeters per tick, out of the engine.
  Depends on: Lab's *Stall detection and remedies in i4sim*, which settles whether making way and a temporary motion fallback belong in the shared part.
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
- **Vec2 equality in place of component comparisons** · simulation, tests, with Lab · added 2026-11-24
  Since #52, `kinetra::Vec2` has `==`, and Lab's Dotio uses it. Kinetra still compares the components one by one in `same(Vec2, Vec2)` in `src/simulation/physics.cpp` and in `tests/runtime_tests.cpp:250`; both can use `==`. Lab's item of the same name covers i4sim, Demo and the capture tool.
  Where: [Dotio on `kinetra::Vec2`, open points](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-24-dotio-on-kinetra-vec2.md#open-points).

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
