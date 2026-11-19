# Static window titles

Status: implemented · Branch: `feat/static-window-titles` · Related: [Lab design](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-19-static-window-titles.md), [launcher, sessions and overlays](../engine/launcher-overlays.md)

The window title of a Kinetra application carries status text ("Kinetra | Start menu", "i4sim | factory | RUNNING", Demo and Dotio titles of a hundred characters). The user wants the title to name only what runs: "Launcher" in the menu, the application's name in a session. This document covers the engine half: the launcher's menu title and the `Session::title()` contract.

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

- `run_launcher` titles the menu "Kinetra | Start menu" ([launcher.cpp](../../src/engine/launcher.cpp), in the title refresh of `run_launcher`).
- `Session::title()` is documented only as "Window title for the running session" ([session.h](../../include/kinetra/session.h)), so applications use it as a status line. Lab's titles run past the window's width and change every 500 ms.

## Goals and non-goals

- Goal: the menu title is "Launcher"; the `Session::title()` contract says the title is the application's name.
- Non-goal: moving status into the window. Applications already draw their state; the user chose to drop the title-only details.
- Goal: the title is right from the first frame. While writing the test, `run_launcher` turned out to set it only on a transition or after 500 ms, so the constructor's title ("Kinetra" in Lab's launcher) showed first.
- Non-goal: dropping the periodic refresh. It costs nothing with constant titles and keeps a consumer whose title does change working.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Title text | "Launcher" in the menu, the application's name in a session | User's request |
| Title-only status in Lab | Dropped, not moved to a HUD | User's choice; smallest change |
| Where the menu title lives | Public constant `kinetra::launcher_window_title` in `launcher.h` | Names the value once; hosts and tests can refer to it. Rejected: a `run_launcher` parameter (no consumer needs another text) |

## Design

`launcher.h` declares `inline constexpr std::string_view launcher_window_title = "Launcher"`, and `run_launcher` uses it when no session runs. The `Session::title()` summary now states the contract: the application's name only, with state drawn in the window. Kinetra has no session of its own whose title changes.

`run_launcher` keeps the time of the last title update in a `std::optional`, empty before the first frame, so the first frame always sets the title. The 500 ms interval is the named constant `title_refresh`.

## Affected areas

- [include/kinetra/launcher.h](../../include/kinetra/launcher.h): new constant.
- [include/kinetra/session.h](../../include/kinetra/session.h): `title()` contract.
- [src/engine/launcher.cpp](../../src/engine/launcher.cpp): menu title, title on the first frame.
- [tests/runtime_tests.cpp](../../tests/runtime_tests.cpp): new graphics case.
- [docs/engine/launcher-overlays.md](../engine/launcher-overlays.md): what the title shows.
- Lab: every session's `title()` and the tests that read state from it (Lab design document).

## Test plan

| Behavior | Test | Level |
| --- | --- | --- |
| The menu shows "Launcher" from the first frame | `kinetra_engine_graphics:run_launcher titles the window after the menu or the running session from the first frame`, which reads `SDL_GetWindowTitle` | Graphics |
| An initial session shows its own title from the first frame | Same case | Graphics |
| A menu request restores "Launcher" | Same case | Graphics |

Written first: it failed 2 of 8 assertions on the first-frame checks before the fix. Verification level Full: `cmake --workflow --preset ci` plus the graphics suite, and the Lab Full level through the pointer update.

## Review

Self-review of `git diff main...HEAD`:

- The 500 ms refresh stays although titles are now constant. Kept on purpose (non-goal above); its comment says why.
- `Session::title()` says hosts may refresh less often than every frame. That matches `run_launcher`; `run_session` still sets it every frame, which the cached `Runtime::title` makes free.
- No findings left open.

## Verification

| Command | Result |
| --- | --- |
| `ctest --preset graphics -R "titles the window"` before the fix | Failed, 2 of 8 assertions (expected) |
| `cmake --workflow --preset ci` | 215/215 passed, no warnings |
| `ctest --preset graphics` | 223/224 passed. `input drawing and resizing reach SDL and OpenGL` also fails on `main` in this WSLg environment (synthetic click and wheel events do not arrive); unrelated |
| `clang-format --dry-run --Werror` on the changed sources | Clean |
| `python3 tools/check_docs.py` | All links resolve |

## Pull request

Title: `feat(launcher): title the menu window "Launcher"`

**Summary.** The window title names what is on screen: "Launcher" in the menu, the application's name in a session. `run_launcher` now sets the title on the first frame, not after 500 ms.

**Changes.**
- `launcher.h`: `launcher_window_title`.
- `session.h`: `title()` contract is the application's name only.
- `launcher.cpp`: menu title; title on the first frame.
- `runtime_tests.cpp`: graphics case for the first-frame and menu titles.
- `launcher-overlays.md`: what the title shows.

**Design.** [docs/design/2026-11-19-static-window-titles.md](docs/design/2026-11-19-static-window-titles.md)

**Verification.** `cmake --workflow --preset ci`: 215/215. Graphics: 223/224; the failing `input drawing and resizing` case fails on `main` too in WSLg. The new case failed before the fix.

**Review notes.** The title text is user-visible behavior; the 500 ms refresh is kept on purpose.

**Dependencies and merge order.** Merge first, with a merge commit. Kinetra-Lab's `feat/static-window-titles` points at this branch.

**Follow-up.** None.

## Open points

None.
