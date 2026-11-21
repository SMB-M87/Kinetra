# CI falls back to current Arch packages

Status: implemented · Branch: `ci/archive-mirrors` · Related: [testing, continuous integration](../testing.md#continuous-integration), `docs/stacked-pull-requests` (#20, stacked on this branch)

CI installs its pinned toolchain from one host, `archive.archlinux.org`. On 2026-11-21 that host stopped answering GitHub's runners from 18:11 UTC, and was still down at 23:49; every pull request in both repositories failed in its first step. This change keeps the archive as the normal source and falls back to the current Arch packages when it does not answer, with a warning that the run is not pinned.

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

- `ci.yml` writes one `Server` line, for `archive.archlinux.org`. When that host does not answer, `pacman -Syyuu` stops with `failed retrieving file 'core.db' from archive.archlinux.org : Connection timed out after 10000 milliseconds`.
- On 2026-11-21 this failed Kinetra #20 and #21 (also on their re-runs at 18:14 and 23:49) and Kinetra-Lab #65 (18:12 and 18:34). CI had passed with the same snapshot at 12:11 that day.
- A first version of this branch listed mirrors of the archive from the agent's memory (`europe`, `america` and `asia.archive.pkgbuild.com`). They do not exist (`Could not resolve host`), as this pull request's first run showed.

## Goals and non-goals

- Goal: a pull request can be checked while the archive is down.
- Goal: the pinned toolchain stays the normal case, and a run that is not pinned says so.
- Non-goal: Kinetra-Lab's release workflow. A release should come from the pinned toolchain, so it keeps waiting for the archive.
- Non-goal: a CI image with the toolchain built in. It would remove the dependency on the archive altogether, but it can only be built while the archive answers; a possible follow-up.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Fallback source | The `archlinux` image's own mirror list, saved before CI replaces it | Suggested by the user. It names no host, so it cannot repeat the first version's mistake, and the image works with it out of the box |
| Pinning | The archive first; the fallback only when that install fails | Warnings are errors and the format check compares exact output; a newer compiler or clang-format can change both. The fallback trades that guarantee for a working check during an outage |
| Signal | A `Toolchain not pinned` warning annotation, and the versions printed by the step | The reviewer sees on the pull request that the run used other versions than the snapshot |
| No hard version guard | Not added | A guard that fails on any version change would keep CI red whenever the current packages moved past the snapshot, which defeats the fallback. A visible warning is enough; the next run with the archive checks the pinned versions again |

## Design

The install step saves the image's mirror list, writes the archive's line and runs `pacman -Syyuu`. If that fails, it prints the warning, restores the image's list and installs again from the current repositories. The header comment and [testing](../testing.md#continuous-integration) describe the fallback.

## Affected areas

`.github/workflows/ci.yml`, `docs/testing.md`, this document and the design index.

## Test plan

- The run block parses as YAML and as `sh` (`sh -n`).
- This pull request's CI while the archive is down: the step prints the warning, installs from the image's mirrors, and the format, docs, headless, full and consumer steps run.
- Once the archive answers again: the step installs from the snapshot without a warning.

## Review

- Under `sh -e`, a failing command in an `if` condition does not end the script, so the fallback runs.
- If the image's mirror list had no active server, the second install would fail with a clear pacman error; nothing passes silently.

## Verification

- `python3 -c "import yaml; ..."` parses the workflow; `sh -n` accepts the run block.
- `python3 tools/check_docs.py`: all links resolve.
- CI on this pull request: see the pull request.

## Pull request

**ci: fall back to current Arch packages when the archive is down**

- **Summary:** CI keeps checking pull requests when `archive.archlinux.org` does not answer: it installs the current Arch packages instead, with a *Toolchain not pinned* warning. The archive stays the normal source.
- **Changes:** install step and header comment of `ci.yml`; one sentence in testing. Replaces the first commit's mirror names, which do not exist.
- **Design:** `docs/design/2026-11-21-ci-mirror-fallback.md`.
- **Verification:** YAML and `sh -n`; this pull request's CI.
- **Dependencies and merge order:** bottom of the stack: this pull request, then #20, then #21, each with a merge commit. Kinetra-Lab #65 makes the same change to its `ci.yml`.
- **Follow-up:** possibly a CI image with the toolchain built in.

## Open points

- A fallback run is not pinned: a newer compiler or clang-format can report what the snapshot would not, or the other way round. The warning makes that visible.
