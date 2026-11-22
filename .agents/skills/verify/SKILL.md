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

A changed submodule pointer (`external/Kinetra` in Lab's diff) counts as an engine change, unless Kinetra changed only Markdown, docs or skills between the two pointers (`git -C external/Kinetra diff --name-only <old> <new>`): then it is the Docs level, and a pointer-only pull request runs only Lab's docs check in CI.

## 2. Classify each file; the highest level wins

| Files | Level |
| --- | --- |
| `*.md`, `.agents/**`, `AGENTS.md`, `docs/**`, `tools/README.md` | Docs |
| `.cpp` or private `.h` within one module, or comment-only edits in code | Targeted (comment-only edits: Docs) |
| `include/kinetra/**`, any `CMakeLists.txt`, `build.sh`, `.clang-format`, `tools/*.py`, dependency files | Full |
| Files in more than one module, the submodule pointer (unless its Kinetra range is only Markdown, docs or skills, see above), runtime/launcher sources (`src/runtime/**`, `launcher/**`, presentation `session.cpp`) | Full |
| Last verification of a branch that changed code | Full |

An explicit argument (`docs`, `targeted`, `full`) overrides the result, but never downward when code changed: say so if asked.

Comment-only means the diff of a code file contains only comment lines (`git diff -U0 <file>`). When in doubt, it is not comment-only.

## 3. Run

**Docs.** Run `python3 tools/check_docs.py` (in Kinetra) or `python3 external/Kinetra/tools/check_docs.py` (in Lab). Fix broken links, then stop. No build.

**Targeted.** Reuse the `dev` preset build directory (`build-dev`); if it does not exist, configure it once with `cmake --preset dev`.

```sh
cmake --build --preset dev --target <targets>
ctest --preset dev -R '<regex>'
```

Pick targets and tests from the mapping tables: Kinetra in [testing](../../../docs/testing.md#choosing-targeted-tests), Lab in Lab's `AGENTS.md`.

**Full.**

- Locally only `cmake --workflow --preset graphics` (it sets `SDL_VIDEODRIVER=offscreen` and software Mesa): in Kinetra, and in Lab when a Lab checkout is available. It builds everything and runs every suite, the graphics cases included.
- In Lab, also bounded smoke runs (`--frames 120`) of the launcher, Demo, Dotio and i4sim.
- Do not run `ci`, `headless` or `windows` locally: the pull request's CI runs them on every push (with format, docs and the external consumer). Run one locally only to reproduce a CI failure.
- After pushing, read the pull request's checks (`gh pr checks <number>`). The change is verified when the local graphics run passed and the checks are green; until then report "pushed, CI pending".

## 4. Report

Give a short table with one row per check: command, result (`passed` / `failed` / `not run`), and a note (reason, graphics backend, failing test names). On failure, show the relevant output lines, not the whole log. Never call the change verified when a required check failed or did not run.
