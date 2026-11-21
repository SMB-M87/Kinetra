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

Started on 2026-11-21. The Lab [quality roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-18-quality-roadmap.md) still runs phases 5 to 8; its engine parts are listed here.

## Now

- **Navigation search for Dotio** · navigation, with Lab · added 2026-11-21
  Lab quality-roadmap phase 6 replaces Dotio's own A* with `kinetra::Navigation`; Kinetra gets the search Dotio needs.
  Where: the phase 6 branches (`refactor/dotio-navigation`), with their own design documents.

## Next

- **Skill READMEs** · docs · added 2026-11-21
  `feature`, `grilling`, `verify`, `doc-module`, `refactor`, `adr` and `pr-feedback` still have only a `SKILL.md`; each gets a README from the [template](documentation.md#skill-readme-template), linked from the [skills overview](../.agents/skills/README.md).
  Next step: one branch `docs/skill-readmes` for all seven. Where: Lab quality roadmap, documentation cleanup.

## Later

- **Shared passing mechanism** · navigation, with Lab · added 2026-11-21
  Move the proven, application-free part of i4sim's predictive passing (conflict prediction, lane agreement, maneuver guard) into Kinetra, with a design document and an ADR. Decide whether the four steering models can share one agent-motion interface, and move i4sim-shaped defaults out of the engine.
  Depends on: Lab phases 5, 6 and 7. Where: [Lab quality roadmap, phase 8](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-18-quality-roadmap.md#phase-8-shared-passing-mechanism).
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
