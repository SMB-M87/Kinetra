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

Nothing agreed next in Kinetra itself.

## Later

- **Shared passing mechanism** · navigation, with Lab · added 2026-11-21
  Evaluate whether the i4sim stall remedies call for shared maneuver state or additional motion primitives. The geometry and explicit steering tuning are described in [Shared predictive passing](design/2026-11-26-shared-passing.md).
  Depends on: Lab's *Stall detection and remedies in i4sim*, which settles whether making way and a temporary motion fallback belong in the shared part.
- **CI image with the toolchain built in** · CI, with Lab · added 2026-11-22
  The Ubuntu test jobs install packages and the pinned formatter on each run. A prebuilt Ubuntu image could shorten setup and reduce dependence on the package mirrors.
  Next step: measure setup time and decide how the image would pick up Ubuntu security updates and formatter version changes.
- **Frame read-back on `Runtime`** · runtime · added 2026-11-21
  A helper that returns the current frame's pixels, so Lab's capture tool and the graphics tests stop calling `glReadPixels` themselves.
  Where: [capture tool open points](https://github.com/SMB-M87/Kinetra-Lab/blob/main/tools/capture/README.md#open-points).
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
