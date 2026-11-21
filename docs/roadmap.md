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

Within each section the first item comes first. The order was proposed on 2026-11-21 together with the [Kinetra-Lab roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md), which is the plan for the applications; Lab's quality roadmap of 2026-11-18 is closed, and what it left is listed here and there.

## Now

Nothing in progress in Kinetra itself.

## Next

- **Pull request review skill** · skills, with Lab · added 2026-11-21
  A `pr-review` skill: an agent reviews an open pull request (diff, design document, rules, CI) and posts inline comments and one summary review with a conclusion and points of attention. It never approves or merges. The agent that opened the pull request processes the comments with `pr-feedback` and merges them up its [stack](workflow.md#stacked-pull-requests). Agents post with the user's GitHub account, which cannot approve or request changes on its own pull request, so every review is a comment review.
  Next step: a design session (`grilling`): how the author's session hears about a review (in Claude Code on the web a session can subscribe to a pull request's activity; locally `/loop` or a request), how agent comments are told apart from the user's, and how the skill works with both `gh` and the GitHub tools of a cloud session, which has no `gh`. Where: the user's request of 2026-11-21, Lab item of the same name.
- **Graphics tests in CI** · tests, CI, with Lab · added 2026-11-21
  CI compiles the graphics tests but does not run them, for want of a GPU ([testing](testing.md#continuous-integration)). The `graphics` preset already renders with SDL's offscreen driver and software Mesa ([development](development.md#graphics-execution)), which needs no GPU, so CI could run `kinetra_engine_graphics` on every pull request. Runtime and overlay changes would then be checked on GitHub, also when they come from a cloud session without a display.
  Next step: run the `graphics` workflow in CI's Arch image and see which cases pass; the WSL2 mouse-input failure ([testing, open points](testing.md#open-points)) shows that injected input needs a look.
- **Agent inspector overlay** · runtime, with Lab · added 2026-11-21
  A reusable overlay that shows the rows of one selected agent (a title, label–value rows in groups) beside it or in a corner, so the user sees an agent's motion settings, goal and state while it runs. Kinetra owns layout, drawing and input capture, as for every overlay; applications supply the selection and the rows as headless values, as `DebugEntry` does for the debug menu ([debug_view.h](../include/kinetra/runtime/debug_view.h)).
  Next step: design it with Lab's item of the same name, which starts with i4sim.
- **Skill READMEs** · docs · added 2026-11-21
  `feature`, `grilling`, `verify`, `doc-module`, `refactor`, `adr` and `pr-feedback` still have only a `SKILL.md`; each gets a README from the [template](documentation.md#skill-readme-template), linked from the [skills overview](../.agents/skills/README.md).
  Next step: one branch `docs/skill-readmes` for all seven. Where: [skills overview, open points](../.agents/skills/README.md#open-points).
- **Linux beyond Arch** · build, docs, CI, with Lab · added 2026-11-21
  Kinetra names Artix/Linux its desktop target and documents only `pacman` ([AGENTS.md](../AGENTS.md#identity-and-scope), [development](development.md#dependencies-and-options)). Support Linux in general: dependency commands per distribution family (Arch and Artix, Debian and Ubuntu, Fedora) and a second CI job on a non-Arch distribution, while the pinned Arch toolchain stays the reference that CI's warnings and formatting follow.
  Next step: build and test Kinetra on Ubuntu 24.04 and Fedora and fix what breaks. Lab's part waits for its golden tests to hold on every toolchain.

## Later

- **Shared passing mechanism** · navigation, with Lab · added 2026-11-21
  Move the proven, application-free part of i4sim's predictive passing (conflict prediction, lane agreement, maneuver guard) into Kinetra, with a design document and an ADR; factory policy (traffic zones, parking, junction rules) stays in i4sim. Decide whether the four overlapping steering models (`steer`, `priority_steer`, `tangent_steer`, `avoid_collisions`, which take different inputs and return acceleration or velocity) can share one agent-motion interface, and move i4sim-shaped defaults, such as the `PrioritySteering` values in millimeters per tick, out of the engine.
  Depends on: Lab's *Stall detection and remedies in i4sim*, which settles whether making way and a temporary motion fallback belong in the shared part.
- **Engine performance** · simulation, with Lab · added 2026-11-21
  Optimize the engine code that Lab's benchmarks spend their time in (spatial queries, steering, weighted-grid search, contact response), one hotspot per pull request, with a benchmark before and after and the golden lines unchanged.
  Depends on: Lab's *Performance baseline and hotspots*.
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
