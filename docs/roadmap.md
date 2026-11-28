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

## Next

- **Speed up shared simulation paths in long headless runs** · simulation, with Lab · added 2026-11-27
  Use reproducible Lab runs to find where the engine spends time in steering, collision checks and spatial queries. Reduce per-tick cost and verify that faster runs preserve the expected behavior. The engine already supplies the typed rate controls, frame pacing and tick budget used by windowed labs.
  Next step: profile a million-tick i4sim run after Lab's surroundings optimization, isolate the busiest engine path and measure before and after with the same layout, seeds and settings. Use Demo or Dotio to check the impact where they use the same path. See [engine timing](engine/runtime.md#sessions-and-timing) and [Lab's performance roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md#next).

## Later

- **No default footer position in `MenuLayout`** · launcher · added 2026-11-27
  `MenuLayout::footer_center(float x = 150)` defaults to EXIT on a page without LAUNCHER; on a page with LAUNCHER, 150 is the gap between LAUNCHER and EXIT, so a caller that clicks EXIT through the default misses without an error. Drop the default and let callers take the place from `LaunchMenu::footer_x()`, which Kinetra-Lab's tour and tests already do. Found in the review of #70.
- **Shared steering force diagnostics** · simulation, runtime, with Lab · added 2026-11-27
  Identify which steering and avoidance solvers can expose the applied acceleration and its components so Lab can display them consistently. Keep optional diagnostics out of ordinary headless runs where possible. Share readable screen-space arrow geometry, including a visible arrowhead, across applications. If the force alone cannot explain a predicted encounter, evaluate whether diagnostics can expose the neighbor that contributed to it.
  Next step: trace the existing force outputs, starting with `SteeringForces` and reciprocal avoidance, and compare them with the values currently drawn in each lab. See [steering API](../include/kinetra/simulation/steering.h) and [Lab's debug drawing follow-up](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md#next).
- **Measure and improve weighted route search** · navigation, with Lab · added 2026-11-27
  Separate route-search cost from the Lab's occupancy weights and local clearance policy. Compare expanded nodes, execution time and route availability on identical weighted and unweighted inputs. Consider a work budget if the measurements show it helps. Measure the effect on route validity and on contacts and blockages in Lab runs.
  Next step: profile i4sim's current route search on a long run and turn a representative costly search into a headless engine fixture. See [weighted grid API](../include/kinetra/simulation/weighted_grid.h) and [Lab's routing comparison](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md#later).

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
