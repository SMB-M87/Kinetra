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

For a changed `external/Kinetra` pointer, inspect `git -C external/Kinetra diff --no-renames --name-only <old> <new>` and use Lab's verification rules to decide between Docs and Full. Use Full when the comparison is inconclusive. [Verification in Lab](https://github.com/SMB-M87/Kinetra-Lab/blob/main/AGENTS.md#verification-in-lab)

## 2. Classify each file; the highest level wins

| Files | Level |
| --- | --- |
| `*.md`, `.agents/**`, `AGENTS.md`, `docs/**`, `tools/README.md` | Docs |
| `.cpp` or private `.h` within one module, or comment-only edits in code | Targeted (comment-only edits: Docs) |
| `include/kinetra/**`, any `CMakeLists.txt`, `build.sh`, `.clang-format`, `tools/*.py`, dependency files | Full |
| Files in more than one module, Lab pointer updates rated Full above, runtime/launcher sources (`src/runtime/**`, `launcher/**`, presentation `session.cpp`) | Full |
| Last verification of a branch that changed code | Full |

An explicit argument (`docs`, `targeted`, `full`) overrides the result, but never downward when code changed: say so if asked.

Comment-only means the diff of a code file contains only comment lines (`git diff -U0 <file>`). When in doubt, it is not comment-only.

## 3. Run

**Copies, at every level.** Run `python3 tools/check_copies.py` (in Lab `python3 external/Kinetra/tools/check_copies.py`), with a `--term` for the old wording, each key word and the name of each part of every rule, procedure or term the change rewords ([rule](../../../AGENTS.md#repository-documentation)). It lists the CI jobs, CMake options, presets, flags and inline names the diff removes that a page still names, and every place an old wording is left. Judge each hit: change a stale copy in the same change, and say in the report why a hit stays. Then run `git grep -n -i '<word>'` (in Lab also over `external/Kinetra`) for each key word the change moves, renames or makes stale on its own, and judge those hits the same way: the script reads only Markdown and YAML and finds a phrase only in the word order given. It reads only, so it costs seconds.

**Docs.** Run `python3 tools/check_docs.py` (in Kinetra) or `python3 external/Kinetra/tools/check_docs.py` (in Lab). Fix broken links, then stop. No build.

**Targeted.** Reuse the `dev` preset build directory (`build-dev`); if it does not exist, configure it once with `cmake --preset dev`.

```sh
cmake --build --preset dev --target <targets>
ctest --preset dev -R '<regex>'
```

Pick targets and tests from the mapping tables: Kinetra in [testing](../../../docs/testing.md#choosing-targeted-tests), Lab in Lab's `AGENTS.md`.

**Full.**

- Locally, the Targeted checks of every changed area, so the push compiles and its tests pass. For runtime, launcher or presentation changes, the graphics cases of that area are optional quick feedback: `ctest --preset graphics -L graphics -R '<regex>'` after building its targets in the `graphics` preset (offscreen SDL, software Mesa).
- Use pull request CI for the full presets. Ubuntu 24.04 LTS runs format, docs, `headless` and `ci`, including the graphics tests. Kinetra also runs the external consumer check. Lab runs application smoke tests and a native Windows job. Run a full preset locally to reproduce a CI failure.
- After pushing, read the pull request's checks (`gh pr checks <number>`). The change is verified when the checks are green; until then report "pushed, CI pending".
- When the change alters what a scene shows, take a screenshot of it with Lab's capture tool, look at it and add it to the pull request; the reviewer runs nothing. Refresh the captures that the media guides' tables name for the change with `tools/capture/capture_media.sh`, look at every refreshed file and commit them with the change, and no others ([rule](../../../AGENTS.md#repository-documentation)).
- A change that depends on desktop feel, a real GPU driver or performance also needs the user's playtest, which software rendering cannot replace. Report it as open for the user, who plays it after the merge.

## 4. Report

Give a short table with one row per check, the copies check included: command, result (`passed` / `failed` / `not run`), and a note (reason, graphics backend, failing test names). On failure, show the relevant output lines, not the whole log. Never call the change verified when a required check failed or did not run.
