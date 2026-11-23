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

A changed submodule pointer (`external/Kinetra` in Lab's diff) counts as an engine change, unless Kinetra changed only Markdown, `docs/`, `.agents/`, `.github/` or `tools/` between the two pointers (`git -C external/Kinetra diff --name-only <old> <new>`), which Lab's build does not read: then it is the Docs level, and Lab's CI runs only the docs check for it.

## 2. Classify each file; the highest level wins

| Files | Level |
| --- | --- |
| `*.md`, `.agents/**`, `AGENTS.md`, `docs/**`, `tools/README.md` | Docs |
| `.cpp` or private `.h` within one module, or comment-only edits in code | Targeted (comment-only edits: Docs) |
| `include/kinetra/**`, any `CMakeLists.txt`, `build.sh`, `.clang-format`, `tools/*.py`, dependency files | Full |
| Files in more than one module, the submodule pointer (unless its Kinetra range is only Markdown, `docs/`, `.agents/`, `.github/` or `tools/`, see above), runtime/launcher sources (`src/runtime/**`, `launcher/**`, presentation `session.cpp`) | Full |
| Last verification of a branch that changed code | Full |

An explicit argument (`docs`, `targeted`, `full`) overrides the result, but never downward when code changed: say so if asked.

Comment-only means the diff of a code file contains only comment lines (`git diff -U0 <file>`). When in doubt, it is not comment-only.

## 3. Run

**Copies, at every level.** Run `python3 tools/check_copies.py` (in Lab `python3 external/Kinetra/tools/check_copies.py`), with `--term '<old wording>'` for each rule, procedure or term the change rewords. It lists the CI jobs, CMake options, presets, flags and inline names the diff removes that a page still names, and every place an old wording is left. Judge each hit: change a stale copy in the same change, and say in the report why a hit stays. It reads only, so it costs seconds.

**Docs.** Run `python3 tools/check_docs.py` (in Kinetra) or `python3 external/Kinetra/tools/check_docs.py` (in Lab). Fix broken links, then stop. No build.

**Targeted.** Reuse the `dev` preset build directory (`build-dev`); if it does not exist, configure it once with `cmake --preset dev`.

```sh
cmake --build --preset dev --target <targets>
ctest --preset dev -R '<regex>'
```

Pick targets and tests from the mapping tables: Kinetra in [testing](../../../docs/testing.md#choosing-targeted-tests), Lab in Lab's `AGENTS.md`.

**Full.**

- Locally, the Targeted checks of every changed area, so the push compiles and its tests pass. For runtime, launcher or presentation changes, the graphics cases of that area are optional quick feedback: `ctest --preset graphics -L graphics -R '<regex>'` after building its targets in the `graphics` preset (offscreen SDL, software Mesa).
- Do not run `ci`, `headless`, `graphics` or `windows` as a whole locally: the pull request's CI runs them on every push (with format, docs, the external consumer, a second Linux job on Ubuntu LTS and, in Lab, the smoke runs and Windows), the graphics cases included. Run one locally only to reproduce a CI failure.
- After pushing, read the pull request's checks (`gh pr checks <number>`). The change is verified when the checks are green; until then report "pushed, CI pending".
- When the change alters what a scene shows, take a screenshot of it with Lab's capture tool, look at it and add it to the pull request; the reviewer runs nothing.
- A change that depends on desktop feel, a real GPU driver or performance also needs the user's playtest, which software rendering cannot replace. Report it as open for the user.

## 4. Report

Give a short table with one row per check, the copies check included: command, result (`passed` / `failed` / `not run`), and a note (reason, graphics backend, failing test names). On failure, show the relevant output lines, not the whole log. Never call the change verified when a required check failed or did not run.
