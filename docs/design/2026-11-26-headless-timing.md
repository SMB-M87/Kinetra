# Headless timing outside `runtime/`

Status: implemented · Branch: `refactor/headless-timing` (Kinetra), `ccr-28fc7b5d-y3iaft` (Kinetra-Lab) · Related: [include layout](2026-11-20-include-layout.md#open-points), [ADR 0015](../decisions/0015-include-layout.md), [roadmap](../roadmap.md#now)

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
- Two more Lab logic headers include `runtime/`: `i4sim/controller.h:12` and `kinetra_lab/input.h:3` include `runtime/engine.h`, only for `Input`, `Key`, `Color` and `Camera2D`, which the include layout moved to `view/` (found by the agent review, round 1).
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
- Kinetra-Lab: Demo's and Dotio's `application.h` and i4sim's `simulation_core.h` include `simulation/clock.h`; Dotio's `application.h` and Demo's presentation `session.cpp` also include `view/timing.h` for `RateCounter`. Demo's `main.cpp` and `application_tests.cpp`, which use `FixedClock` too, include `clock.h` themselves instead of reaching it through `application.h`.
- Kinetra-Lab: `kinetra_lab/input.h` includes `view/input.h`, and i4sim's `controller.h` includes `view/input.h` and `view/camera.h`, in place of `runtime/engine.h`.

## Affected areas

- Kinetra headers: [clock.h](../../include/kinetra/simulation/clock.h) (new), [view/timing.h](../../include/kinetra/view/timing.h) (moved), [engine.h](../../include/kinetra/runtime/engine.h).
- Kinetra tests: `simulation_tests.cpp`, `presentation_tests.cpp` and the consumer test's include lines.
- Kinetra docs: README module table, [simulation](../../src/simulation/README.md) and [runtime](../../src/runtime/README.md) READMEs, [architecture](../architecture.md), [runtime guide](../engine/runtime.md), [launcher and overlays](../engine/launcher-overlays.md), [testing](../testing.md), [roadmap](../roadmap.md).
- Kinetra-Lab: the include lines of Demo, Dotio and i4sim (with `main.cpp` and `application_tests.cpp`), of i4sim's `controller.h` and of `kinetra_lab/input.h`; Dotio's open point, the roadmap, and the pointer.

## Test plan

| Behavior | Test | Level |
| --- | --- | --- |
| Behavior unchanged | Every existing suite, unchanged apart from include lines: `kinetra_behavior` (`FixedClock`) and `kinetra_presentation` (`RateCounter`, `FrameTimes`, `FrameLimiter`) | Full |
| Both headers build without the runtime | The consumer test includes `simulation/clock.h` and `view/timing.h`; Kinetra's `headless` preset and Lab's `-DKINETRA_LAB_BUILD_RUNTIME=OFF` logic build | Full |
| No old path remains | `grep -rn 'runtime/timing.h'` over the code and current docs of Kinetra and Lab returns nothing outside design documents and ADRs | Manual |
| No Lab logic includes `runtime/` | `grep -rln 'kinetra/runtime/' labs/*/include labs/*/src labs/shared/include`, without `presentation/` and `main.cpp`, lists only `kinetra_lab/inspector.h`, `settings.h` and `session.h`, the runtime side of the shared policy; no logic file includes those three | Manual |
| Docs links and copies | `python3 tools/check_docs.py`; `check_copies.py --term 'runtime/timing.h' --term 'timing.h'` | Docs |

A refactor: no new tests and no changed assertions.

## Review

The whole diff of both branches was read as a reviewer would.

- **Includes.** `view/timing.h` keeps every standard header its three types use (`<algorithm>` for `std::max`, `<array>`, `<cmath>`, `<cstddef>`, `<stdexcept>`); `clock.h` takes only what `FixedClock` needs. Neither includes another Kinetra header, so both stay headless.
- **No hidden users.** No runtime source uses `FixedClock`, and every Lab file that names `FixedClock` or `RateCounter` includes the header that declares it (Demo's `main.cpp` and `application_tests.cpp` since round 1); the headless and graphics builds of Lab, which compile every application and test, show no file that relied on `engine.h` for `FixedClock`.
- **Docs.** The README module table also gains `passing.h` and `inspector.h`, which it had left out; the simulation README gets a `clock.h` row and a node in its structure diagram. Historical design documents keep their text and only had link targets updated, as the include layout did.
- **Copies.** `check_copies.py --term 'runtime/timing.h' --term 'timing.h' --term 'FixedClock' --term 'stays with the runtime' --term 'clock.h'` from Kinetra-Lab: no page still names the old path outside dated design documents; every other hit names the new path or the type without a path.

### Round 1 (agent review)

| Finding | Resolution | Commit |
| --- | --- | --- |
| Blocking: the goal *No Kinetra-Lab logic header includes `runtime/`* was not met; `i4sim/controller.h` and `kinetra_lab/input.h` still included `runtime/engine.h`, and the `grep` for `runtime/timing.h` could not show it | Both include the `view/` headers they use; the goal is proven by a search for `kinetra/runtime/` (*Test plan*) | Kinetra-Lab e903c3c |
| Nit: the frame-times design document labelled its `clock.h` link `timing.h` | Labelled `clock.h` | this commit |
| Nit: README's Runtime row left out `inspector_view.h` | Added | this commit |
| Nit: the simulation README's purpose and the architecture page's `Headless` node named neither the fixed clock nor passing | Both named | this commit |
| Nit: Demo's `main.cpp` and `application_tests.cpp` reached `FixedClock` through `application.h` | They include `clock.h` | Kinetra-Lab e903c3c |
| Lesson: prove a goal with a check as wide as its wording | Added to [review lessons](../review-lessons.md#code) | 7c8e94b |

## Verification

- Kinetra: `cmake --preset headless`, `cmake --build --preset headless`, `ctest --preset headless`: 255/255 passed.
- Kinetra consumer test, headless (`cmake -S tests/consumer -DCONSUMER_GRAPHICS=OFF`), built and run: exit code 0.
- Kinetra-Lab with this branch as its pointer: `cmake --preset headless`, `cmake --build --preset headless` and `ctest --preset headless -LE soak`: 268/268 passed (Demo, Dotio and i4sim logic without the runtime).
- Kinetra-Lab `graphics` preset, everything built (engine runtime with the new `engine.h`, every application, presentation tests and executables): `ctest --preset graphics -LE soak`: 347/347 passed.
- `python3 tools/check_docs.py` in both repositories: all links resolve. `clang-format` on every changed `.h` and `.cpp`: clean.
- `grep -rn 'runtime/timing.h'` over the code and current docs of Kinetra and Lab: only dated design documents, this one included.
- After round 1: `grep -rln 'kinetra/runtime/'` over Lab's `include/` and `src/` outside `presentation/` and `main.cpp` lists only `kinetra_lab/inspector.h`, `settings.h` and `session.h`, and no logic file includes them; Lab `headless` 268/268 and `graphics` 347/347 again (both `-LE soak`).
- The Full level is the pull requests' CI in both repositories.

## Pull request

**refactor(layout): split timing.h into simulation/clock.h and view/timing.h**

Summary: `<kinetra/runtime/timing.h>` is header-only and headless, yet Kinetra-Lab's headless logic had to include it from `runtime/`. `FixedClock`, which steps the simulation, moves to `<kinetra/simulation/clock.h>`; `RateCounter`, `FrameTimes` and `FrameLimiter`, which serve presentation, move to `<kinetra/view/timing.h>`. No behavior changes; the old path goes without a forwarding header, as ADR 0015 chose for the layout.

Changes:
- `include/kinetra/simulation/clock.h` (new), `include/kinetra/runtime/timing.h` → `include/kinetra/view/timing.h`, `engine.h` includes the new path.
- Include lines of `simulation_tests.cpp`, `presentation_tests.cpp` and the consumer test.
- Docs: README module table, simulation and runtime READMEs, architecture, runtime guide, launcher and overlays, testing; roadmap item closed.

Design: `docs/design/2026-11-26-headless-timing.md`.

Verification: see the design document; CI is the Full level.

Dependencies and merge order: this pull request first, with a merge commit; the Kinetra-Lab pull request that follows its includes, and drops `runtime/engine.h` from i4sim's `controller.h` and `kinetra_lab/input.h`, merges after it with its pointer at this branch, and the `lab-pointer` workflow then records the merge commit.

Follow-up: none.

## Open points

None.
