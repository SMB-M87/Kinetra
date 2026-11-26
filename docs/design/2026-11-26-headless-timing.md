# Headless timing outside `runtime/`

Status: accepted · Branch: `refactor/headless-timing` (Kinetra), `ccr-28fc7b5d-y3iaft` (Kinetra-Lab) · Related: [include layout](2026-11-20-include-layout.md#open-points), [ADR 0015](../decisions/0015-include-layout.md), [roadmap](../roadmap.md#now)

`<kinetra/runtime/timing.h>` is header-only and headless, yet the headless logic of Kinetra-Lab's three applications includes it from `runtime/`, the layer it may not use. This change splits it by layer: `FixedClock` moves to `<kinetra/simulation/clock.h>`, and `RateCounter`, `FrameTimes` and `FrameLimiter` move to `<kinetra/view/timing.h>`. No behavior changes.

## Table of contents

- [Problem](#problem)
- [Goals and non-goals](#goals-and-non-goals)
- [Decisions](#decisions)
- [Design](#design)
- [Affected areas](#affected-areas)
- [Test plan](#test-plan)
- [Review](#review)
- [Verification](#verification)
- [Pull request](#pull-request)
- [Open points](#open-points)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem

- The include layout ([design](2026-11-20-include-layout.md#design)) says that Lab logic includes `simulation/` and `view/` only, and that `runtime/` is the layer of `Kinetra::Runtime`. It left `timing.h` in `runtime/` as an open point, because its frame limiter serves the main loop.
- `timing.h` has no runtime dependency: it includes only the standard library, and the `kinetra_behavior` and `kinetra_presentation` suites test it against `Kinetra::Simulation` alone ([CMakeLists.txt](../../CMakeLists.txt)).
- Kinetra-Lab's headless logic includes it from `runtime/`: `demo/application.h:7`, `dotio/application.h:11` and `i4sim/simulation/detail/simulation_core.h:14`. An include line there suggests a runtime dependency that does not exist. Kinetra's `simulation_tests.cpp:4` does the same for `FixedClock`.
- The four types belong to different layers. `FixedClock` turns elapsed time into simulation ticks and is a member of the simulation state of Demo, Dotio and i4sim. `RateCounter` and `FrameTimes` are statistics data for overlays, and `FrameLimiter` decides frame deadlines from given times; all three are headless view helpers.

## Goals and non-goals

Goals:

- No Kinetra-Lab logic header includes `runtime/`.
- Each type sits in the layer that matches its use: `FixedClock` in `simulation/`, the frame helpers in `view/`.
- Docs name the new paths, and the include-layout open point is closed.

Non-goals:

- Any change to the types themselves, their contracts or their tests' assertions.
- A forwarding header at the old path.
- Other header moves.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Where the types go | Split: `FixedClock` to `simulation/clock.h`; `RateCounter`, `FrameTimes` and `FrameLimiter` to `view/timing.h` | User's choice (2026-11-26), over moving the whole header to `view/` or to `simulation/`. Each layer then holds only what it uses: the simulation steps with `FixedClock`, and the frame helpers serve presentation |
| Compatibility | Hard break, no forwarding header | As [ADR 0015](../decisions/0015-include-layout.md) chose for the layout; the only consumers are Kinetra-Lab and the consumer test, both updated in this change |
| ADR | None | The move follows the layer rule that ADR 0015 records; it changes no boundary |
| History | `git mv` `runtime/timing.h` to `view/timing.h`; `clock.h` is new | Three of the four types stay in the moved file, so `git log --follow` keeps their history |
| Design document | One, here | Most of the change is in Kinetra; Kinetra-Lab's pull request links it |

## Design

```text
include/kinetra/
  simulation/  clock.h   FixedClock                          (new)
  view/        timing.h  RateCounter, FrameTimes, FrameLimiter (was runtime/timing.h)
  runtime/     engine.h  includes view/timing.h for Runtime::frame_times()
```

- `clock.h` takes `FixedClock` with its `///` summaries unchanged. It includes only the standard library.
- `view/timing.h` keeps the other three types unchanged. A `view/` header may include `simulation/` and never `runtime/`; this one includes neither.
- `runtime/engine.h` includes `view/timing.h` in place of `runtime/timing.h`, because `Runtime::frame_times()` returns `FrameTimes`. It does not include `clock.h`: no runtime source uses `FixedClock`, so a user of `FixedClock` includes `clock.h` itself.
- Kinetra-Lab: Demo's and Dotio's `application.h` and i4sim's `simulation_core.h` include `simulation/clock.h`; Dotio's `application.h` and Demo's presentation `session.cpp` also include `view/timing.h` for `RateCounter`.

## Affected areas

- Kinetra headers: [clock.h](../../include/kinetra/simulation/clock.h) (new), [view/timing.h](../../include/kinetra/view/timing.h) (moved), [engine.h](../../include/kinetra/runtime/engine.h).
- Kinetra tests: `simulation_tests.cpp`, `presentation_tests.cpp` and the consumer test's include lines.
- Kinetra docs: README module table, [simulation](../../src/simulation/README.md) and [runtime](../../src/runtime/README.md) READMEs, [architecture](../architecture.md), [runtime guide](../engine/runtime.md), [launcher and overlays](../engine/launcher-overlays.md), [testing](../testing.md), [roadmap](../roadmap.md).
- Kinetra-Lab: the include lines of Demo, Dotio and i4sim, Dotio's open point, the roadmap, and the pointer.

## Test plan

| Behavior | Test | Level |
| --- | --- | --- |
| Behavior unchanged | Every existing suite, unchanged apart from include lines: `kinetra_behavior` (`FixedClock`) and `kinetra_presentation` (`RateCounter`, `FrameTimes`, `FrameLimiter`) | Full |
| Both headers build without the runtime | The consumer test includes `simulation/clock.h` and `view/timing.h`; Kinetra's `headless` preset and Lab's `-DKINETRA_LAB_BUILD_RUNTIME=OFF` logic build | Full |
| No old path remains | `grep -rn 'runtime/timing.h'` over the code and current docs of Kinetra and Lab returns nothing outside design documents and ADRs | Manual |
| Docs links and copies | `python3 tools/check_docs.py`; `check_copies.py --term 'runtime/timing.h' --term 'timing.h'` | Docs |

A refactor: no new tests and no changed assertions.

## Review

Not yet reviewed.

## Verification

Not yet run.

## Pull request

Not yet written.

## Open points

None yet.
