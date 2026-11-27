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

- **Stall detection and remedies in the engine** · simulation, with Lab · added 2026-11-27
  `StallDetector`, a `RightOfWay` fallback and a `MakeWay` lifecycle beside `PassingCoordinator` in `Kinetra::Simulation`, each used by i4sim in the same step ([ADR 0018](decisions/0018-stall-remedies-in-the-engine.md)).
  Where: branch `feat/stall-detector` (step 3 of Lab's stack, `StallDetector`), [Lab design](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-27-stall-remedies-in-the-engine.md).

## Next

Nothing agreed next in Kinetra itself.

## Later

Nothing waiting in Kinetra itself.

- **No default footer position in `MenuLayout`** · launcher · added 2026-11-27
  `MenuLayout::footer_center(float x = 150)` defaults to EXIT on a page without LAUNCHER; on a page with LAUNCHER, 150 is the gap between LAUNCHER and EXIT, so a caller that clicks EXIT through the default misses without an error. Drop the default and let callers take the place from `LaunchMenu::footer_x()`, which Kinetra-Lab's tour and tests already do. Found in the review of #70.
- **Say where the mutation check of a contract branch is recorded** · docs, agent rules · added 2026-11-27
  The [rules for tests](testing.md#rules-for-tests) ask that the case for each branch a contract names be checked by mutation, but no page says where the result goes. A reviewer runs nothing, so it finds the check only when the author writes it down, as #72's description did. Name the mutation result in the design template's *Test plan* or *Verification*, and in the `pr-review` checklist row "Every changed behavior has a test".
  Next step: a docs-only pull request on those rule pages, with an agent review. Where: the agent review of [#78](https://github.com/SMB-M87/Kinetra/pull/78).

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
