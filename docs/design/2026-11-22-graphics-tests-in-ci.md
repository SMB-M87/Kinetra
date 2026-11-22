# Graphics tests in CI

Status: implemented · Branch: `build/graphics-in-ci` · Related: [testing, Full](../testing.md#full), [continuous integration](../testing.md#continuous-integration), Lab design `2026-11-22-graphics-tests-in-ci.md` (the application half), [Local verification runs only graphics](2026-11-20-verify-locally-graphics.md) (the rule this one replaces)

CI compiled the graphics tests but did not run them, so the Full level asked for a local graphics run on top of the pull request's checks. The `graphics` preset already renders without a GPU, with SDL's offscreen driver and software Mesa. This change makes CI run the graphics cases on that same backend, and the user decided on 2026-11-22 that CI is then the whole Full level: no local graphics run is required any more.

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

- The `ci` test preset inherited `no-graphics` (`CMakePresets.json`), so CI built `kinetra_engine_runtime_tests` and skipped its 13 cases. The reason in `ci.yml` was "no GPU", but the `graphics` preset never used one: it sets `SDL_VIDEODRIVER=offscreen` and `LIBGL_ALWAYS_SOFTWARE=1`.
- The Full level therefore needed a local `cmake --workflow --preset graphics` ([testing](../testing.md#full), `AGENTS.md`, `verify` and `pr-review` skills). An agent in a cloud session ran a check that CI could have run, and a runtime change from a session that could not run it was not checked at all.
- Measured on 2026-11-22 in a cloud session (Ubuntu 24.04, SDL 2.30, Mesa 25.2 llvmpipe, no GPU, no display): all 13 `kinetra_engine_graphics` cases pass in 0.8 s, including the injected mouse-input case that failed under WSL2 ([testing, open points](../testing.md#open-points)).

## Goals and non-goals

- Goal: CI runs every test, the graphics cases included, on every pull request and push to `main`.
- Goal: the rules say that CI is the Full level; locally only the targeted checks, and graphics cases as optional quick feedback.
- Goal: every copy of the old rule changes: `AGENTS.md`, testing, development, workflow, and the `verify` and `pr-review` skills.
- Goal: the reviewer only reviews code and comments, and starts once CI is green.
- Non-goal: Windows. Lab's native Windows job keeps skipping graphics; a software OpenGL 3.3 context on the Windows runner would be a change of its own.
- Non-goal: desktop feel, real GPU drivers and performance. Software rendering cannot prove them; they stay a playtest by the user.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| The local rule once CI runs graphics | CI is the Full level. Locally only the targeted checks of every changed area; graphics cases optional, as quick feedback or to reproduce a CI failure | The user's choice of 2026-11-22. CI uses the same backend as a local run, so the local run only repeated it |
| Kinetra and Lab | Both, Kinetra first | The user's choice of 2026-11-22. The rule lives here and Lab copies it |
| How CI runs them | The `ci` test preset runs everything, with the offscreen environment; a hidden `software-gl` test preset holds that environment for `graphics` and `ci` | No second build: `build-ci` already has the graphics tests. One step, and one preset that a developer can run to reproduce CI |
| Desktop checks | A design document names a playtest by the user when a change depends on feel, a GPU driver or performance | Neither CI nor a local software run proves them |
| The mouse-input case under `sdl2-compat` | `Runtime::poll` reads `wheel.preciseY`. The first CI run failed it with "Mouse input lost", as WSL2 did. The cause is in `sdl2-compat` 2.32.64's `Event2to3`: a pushed SDL2 wheel event reaches SDL3 with `integer_y` taken from the bytes of `mouseY`, and comes back with `y` equal to that, so 0. `Event3to2` sets `preciseY` from the float `y`, which `Event2to3` took from the whole `y`: an injected whole step arrives in `preciseY`, a fraction does not. | The user's choice of 2026-11-22. `Input::wheel` and `zoom` already promised fractional steps for smooth wheels; a notched wheel gives the same values as before |
| Minimum SDL2 | 2.0.18, in `find_package(SDL2 2.0.18 REQUIRED)` and [development](../development.md#dependencies-and-options) | `SDL_MouseWheelEvent::preciseY` arrived in 2.0.18; without the version, configure succeeds on an older SDL2 and `runtime.cpp` then fails to compile (found by the agent review). The pinned Arch image (`sdl2-compat` 2.32), Ubuntu 24.04 (2.30), MSYS2 and Lab's pinned MinGW archive (2.32.10) all meet it |
| The reviewer | `pr-review` reviews the code and comments, and builds, tests and runs nothing; the author starts it only once the pull request's CI is green, and after new commits it reviews only a green head. A screenshot of a changed scene is the author's; the reviewer checks it is there | The user's choice of 2026-11-22, during this pull request: CI and the author run everything, so a run by the reviewer only repeated it |

## Design

```mermaid
flowchart LR
    Build["ci build<br/>warnings as errors"] --> Tests["ctest --preset ci<br/>SDL offscreen, software Mesa"]
    Tests --> Unit[unit cases]
    Tests --> Graphics["kinetra_engine_graphics<br/>(was skipped)"]
```

*Takeaway:* CI runs the graphics cases in the build it already makes; nothing else in the pipeline changes.

- `CMakePresets.json`: the hidden test preset `software-gl` (base plus `SDL_VIDEODRIVER=offscreen`, `LIBGL_ALWAYS_SOFTWARE=1`); `graphics` and `ci` inherit it, so `ci` no longer excludes the `graphics` label. The `ci` configure preset's display name says so.
- `.github/workflows/ci.yml`: the header comment and the step name.
- Rules: the Full level is CI plus local targeted checks (`AGENTS.md`, [testing](../testing.md#full)); the reviewer reads CI's results and checks that the author added a screenshot of a changed scene ([pr-review](../../.agents/skills/pr-review/SKILL.md), [workflow](../workflow.md#6-review)); `verify` stops asking for a local graphics run.

## Affected areas

`CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/ci.yml`, `AGENTS.md`, `docs/testing.md`, `docs/development.md`, `docs/workflow.md`, `.agents/skills/verify/`, `.agents/skills/pr-review/` (`SKILL.md` and `README.md`), `.agents/skills/feature/SKILL.md`, `.agents/skills/README.md`, `.agents/skills/pr-feedback/README.md`, `docs/review-lessons.md`, `src/runtime/runtime.cpp`, `src/runtime/README.md`, `tests/runtime_tests.cpp`, the roadmap item *Graphics tests in CI* (removed) and *Report the injected wheel event to sdl2-compat* (added), this document and the design index.

## Test plan

| Behavior | Check | Level |
| --- | --- | --- |
| `ctest --preset ci` runs the graphics cases with the offscreen environment | Locally: `cmake --preset ci && cmake --build --preset ci && ctest --preset ci` lists and passes 278 cases, 13 of them `graphics` | Full |
| They pass in CI's pinned Arch image (`sdl2-compat`, Mesa from the snapshot) | This pull request's `checks` job | Full |
| `Runtime::poll` adds `wheel.preciseY`: a whole notch and a flipped notch | The mouse-input case of `kinetra_engine_graphics`. Its wheel check failed with the old `poll` under `sdl2-compat` (CI on 37d9a3d) and passes with the new one | Full |
| A fractional step | No test: `sdl2-compat` sets a pushed event's `preciseY` from its whole `y`, so a half step cannot be injected there (CI on 043f6d6 lost it) | — |
| Smooth-wheel and touchpad zoom on a desktop, in Demo, Dotio and i4sim | A playtest by the user. Every Lab use passes `Input::wheel` as a float to `kinetra::zoom`; none steps once per positive value | Playtest |
| Docs and skills links | `python3 tools/check_docs.py` | Docs |

The pinned Arch image could not be run in the cloud session: its network policy refuses `archive.archlinux.org` and the Arch mirrors, so the Arch result comes from this pull request's CI.

## Review

The agent review on 5231801 (*Fix first*, code review `high`) found:

| Finding | Resolution |
| --- | --- |
| Four copies still started the review after opening the pull request (skills overview twice, the `pr-feedback` guide, the `pr-review` row in `AGENTS.md`) | Changed to "once the pull request's CI is green"; the workflow diagram has a *CI green* node. Search: `grep -rni 'after opening\|after every pull request\|after the pull request'` finds no copy left. A lesson |
| `preciseY` needs SDL2 2.0.18, stated nowhere | `find_package(SDL2 2.0.18 REQUIRED)`, development, a *Decisions* row |
| `verify` says Lab's CI runs the smoke runs; testing does not | Testing's CI list names them; Lab #73 adds them |
| The WSL2 open point was closed without a WSL2 run | Kept open, with the probable cause and a run by the user |
| The `pr-review` checklist still named `tools/check_docs.py` | Points at CI's *Documentation links* step |
| Test plan, *Verification* and *Pull request* behind the diff; the `sdl2-compat` follow-up not on the roadmap | Updated; the roadmap item waits for the user |
| Nits: the screenshot in *Design*, "survives", `clicked` on the next poll | Fixed; the flipped notch follows the held check |

## Verification

| Command | Result |
| --- | --- |
| `cmake --preset ci && cmake --build --preset ci && ctest --preset ci -j4` (Ubuntu 24.04, GCC 13.3, SDL 2.30, Mesa 25.2 llvmpipe, offscreen) on 5231801 | passed, 278 of 278, 13 graphics |
| The same on the review fixes (CMake's SDL2 version check included) | passed, 278 of 278 |
| `python3 tools/check_docs.py` | passed |
| CI on 8d672fc and 37d9a3d (pinned Arch, `sdl2-compat`) | 12 of 13 graphics cases passed; the mouse-input case lost the wheel |
| CI on 043f6d6 | the whole notch passed; the half step was lost |
| CI on 5231801 | passed, 278 of 278, 13 graphics |
| CI on the review fixes | see the pull request |

## Pull request

**Title:** `build(ci): run the graphics tests in CI, make CI the Full level, review code only`

**Summary.** CI compiled the graphics tests but skipped them. The `ci` test preset now runs them with SDL's offscreen driver and software Mesa, the backend the `graphics` preset always used, so they need no GPU. With that, the user decided that CI is the whole Full level: agents no longer run the graphics workflow locally.

**Changes.**
- `CMakePresets.json`: hidden test preset `software-gl` with the offscreen environment; `graphics` and `ci` inherit it.
- `ci.yml`: comment and step name.
- `AGENTS.md`, testing, development, workflow, `verify` and `pr-review`: CI is the Full level; locally the targeted checks, graphics cases optional; a desktop playtest stays with the user.
- `pr-review`, `feature`, workflow and `AGENTS.md`: the reviewer reviews code and comments only, and starts once CI is green; the author adds a screenshot of a changed scene.
- `runtime.cpp`: `Runtime::poll` adds `wheel.preciseY` instead of the whole notches in `wheel.y`, so `Input::wheel` is fractional for smooth wheels, as its contract says.
- `runtime_tests.cpp`: the mouse-input case fills its injected events as SDL does, checks click and wheel separately, and adds a flipped notch. The wheel check fails with the old `poll` under `sdl2-compat` (CI on 37d9a3d). A fractional step cannot be pushed through `sdl2-compat`, which sets a pushed event's `preciseY` from its whole `y`, so no test injects one.
- Roadmap item *Graphics tests in CI* removed.

**Verification.** See [Verification](#verification).

**Dependencies and merge order.** [Kinetra-Lab #73](https://github.com/SMB-M87/Kinetra-Lab/pull/73), the application half, points at this branch and merges after this one, which merges with a merge commit. This pull request changes the agent rules, CI and the runtime, and names a playtest by the user, so it waits for the user.

**Follow-up.** A run under WSL2 by the user ([testing, open points](../testing.md#open-points)); the roadmap item *Report the injected wheel event to sdl2-compat*, which waits for the user.

## Open points

- Smooth wheels and touchpads now zoom in fractional steps (`Input::wheel` from `preciseY`); that is what the contracts of `Input::wheel` and `zoom` already said, but its feel on a desktop is the user's playtest.
- `sdl2-compat` 2.32.64 copies a pushed wheel event's `mouseX`/`mouseY` into SDL3's `integer_x`/`integer_y` (`Event2to3`, a `memcpy` past the timestamp), and reads `y` back from `integer_y`. Real input is not affected. Whether to report it upstream is the user's call (roadmap).
- WSL2 probably failed the mouse-input case for the same reason; a run there by the user confirms it.
