---
name: verify
description: Choose and run the right verification level (Docs, Targeted or Full) for the current changes in Kinetra or Kinetra-Lab, and report commands and results. Use after changing files and before committing or handing over, or when the user says /verify, "controleer", "draai de tests".
argument-hint: "[docs|targeted|full] (optional override)"
---

# Verify by impact

Implements the [verification levels](../../../docs/testing.md#verification-levels). (In Kinetra-Lab, the path is `external/Kinetra/docs/testing.md`.) Never run a full build when only documentation changed, and never skip compiling changed code.

## 1. Collect the change set

Run this in every repository with changes (Lab and/or `external/Kinetra`):

```sh
git diff --name-only main...HEAD      # committed on this branch
git status --short                    # uncommitted and untracked
```

A changed submodule pointer (`external/Kinetra` in Lab's diff) counts as an engine change.

## 2. Classify each file; the highest level wins

| Files | Level |
| --- | --- |
| `*.md`, `.agents/**`, `AGENTS.md`, `docs/**`, `tools/README.md` | Docs |
| `.cpp` or private `.h` within one module, or comment-only edits in code | Targeted (comment-only edits: Docs) |
| `include/kinetra/**`, any `CMakeLists.txt`, `build.sh`, `.clang-format`, `tools/*.py`, dependency files | Full |
| Files in more than one module, the submodule pointer, runtime/launcher sources (`src/engine/**`, `launcher/**`, presentation `session.cpp`) | Full |
| Last verification of a branch that changed code | Full |

An explicit argument (`docs`, `targeted`, `full`) overrides the result, but never downward when code changed: say so if asked.

Comment-only means the diff of a code file contains only comment lines (`git diff -U0 <file>`). When in doubt, it is not comment-only.

## 3. Run

**Docs.** Run `python3 tools/check_docs.py` (in Kinetra) or `python3 external/Kinetra/tools/check_docs.py` (in Lab). Fix broken links, then stop. No build.

**Targeted.** Reuse the existing build directory: Kinetra standalone uses `build`; in Lab, Lab uses `build` and the core uses `build-core`. If it does not exist, configure it once with the commands from the Full level, without running the full test suite.

```sh
cmake --build <build-dir> --target <targets> -j2
ctest --test-dir <build-dir> -R '<regex>' --output-on-failure
```

Pick targets and tests from the mapping tables: Kinetra in [testing](../../../docs/testing.md#choosing-targeted-tests), Lab in Lab's `AGENTS.md`.

**Full.**

- Kinetra only: configure, build and run `ctest` in Kinetra (commands in [testing](../../../docs/testing.md#full)), then `./build.sh` in Lab when a Lab checkout is available.
- Lab: `./build.sh` from the Lab root.
- Runtime, launcher or presentation changes: also the graphics configuration with `KINETRA_TEST_GRAPHICS=ON` and `SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1`, plus bounded smoke runs (`--frames 120`) of the launcher, Demo, Dotio and i4sim.
- Public header or target changes: also the external consumer fixture.

## 4. Report

Give a short table with one row per check: command, result (`passed` / `failed` / `not run`), and a note (reason, graphics backend, failing test names). On failure, show the relevant output lines, not the whole log. Never call the change verified when a required check failed or did not run.
