# CI Linux matrix

Status: implemented · Branch: `build/ci-linux-matrix` · Related: [Linux beyond Arch](2026-11-22-linux-beyond-arch.md), Kinetra-Lab's copy in its pull request #78

CI's two Linux jobs, `checks` on Arch and `ubuntu`, ran the same build and tests with copied steps. They become one job, `linux`, with a matrix entry per distribution, each in its own container; the required `checks` status becomes a small job that passes when every entry passed, so Ubuntu is required too.

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

- The `ubuntu` job of [Linux beyond Arch](2026-11-22-linux-beyond-arch.md) repeats every build and test step of `checks`. Copies drift: Kinetra-Lab's copy of the job missed the CMake path fix that this repository's got in review.
- `ubuntu` ran on the runner itself, so the runner image's newer tools could stand in for Ubuntu's packages; it needed `/usr/bin` first on the path to test what the documented command installs.
- Whether `CI / ubuntu` should be a required status was open (roadmap *Linux support follow-ups*).

## Goals and non-goals

Goals: one definition of the Linux build and tests; every distribution in its own container; Ubuntu required; adding a distribution is one matrix entry.

Non-goals: a Fedora entry (its package names are still unchecked); changing what each distribution checks.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| One job or two | One `linux` job with a matrix (the user, 2026-11-23) | No copied steps; a new distribution is one entry |
| Is Ubuntu required | Yes: `checks` passes only when every `linux` entry passed (the user, 2026-11-23) | A Linux the docs call tested should block a merge when it breaks |
| Ubuntu on the runner or in a container | In an `ubuntu:24.04` container, like Arch | Only the distribution's own packages are present, so no runner tool can stand in for them, and both entries run the same way |
| Keep the status name | `checks` stays, as a job that waits for `linux` | The branch protection on `main` requires `CI / checks`; matrix runs are named `linux (arch)` and `linux (ubuntu)` |

## Design

- `linux` has `fail-fast: false`, so one distribution's failure does not cancel the other's run, and `container: ${{ matrix.image }}`.
- Each entry has its own install step (`if: matrix.distro == …`). Arch keeps the archive snapshot and its fallback; Ubuntu installs the development page's package set plus `git` and `ca-certificates` for the checkout.
- Formatting, the docs check, the icon check and `cmake --workflow --preset ci` with warnings as errors run only for `arch`; the other entries build the `ci` preset with `KINETRA_WARNINGS_AS_ERRORS=OFF` and turn warnings into annotations. Headless, the tool versions and the external consumer run for every entry.
- `checks` (`needs: linux`, `if: always()`) fails unless `needs.linux.result` is `success`. `always()` and not `!cancelled()`: GitHub counts a skipped required job as passed, so on a cancelled run `checks` must still run and fail.

## Affected areas

`.github/workflows/ci.yml`; [testing](../testing.md#continuous-integration) and [development](../development.md#dependencies-and-options), which name the jobs; the [roadmap](../roadmap.md) item *Linux support follow-ups*; the open points of [Linux beyond Arch](2026-11-22-linux-beyond-arch.md#open-points); this document and the [index](README.md#index).

## Test plan

| Behavior | Proof | Level |
| --- | --- | --- |
| Both entries build and pass as before | This pull request's CI: `linux (arch)` and `linux (ubuntu)` green; the Ubuntu run's *Tool versions* shows `/usr/bin/cmake` 3.28 | Full (CI) |
| `checks` reflects every entry | `checks` green only after both; its log prints `linux: success` | Full (CI) |

## Review

- The first CI run failed both entries with exit code 141 on the *Tool versions* step: the explicit `shell: bash` adds `-o pipefail`, and `ldd --version | head -1` died of SIGPIPE when `head` closed the pipe. Every `head -1` became `sed -n 1p` (`f0dd744`), with a [review lesson](../review-lessons.md#ci-and-tooling); Kinetra-Lab#78 got the same fix.

Agent review of `87d7afb` (Fix first), round 1:

| Finding | Resolution |
| --- | --- |
| Should fix: with `if: !cancelled()`, a cancelled run skipped `checks`, and a skipped required job counts as passed | `if: always()`, with a comment why; the *Design* bullet says the same. Kinetra-Lab#78 has the same fix |
| Should fix: testing's *Full* paragraph still named "job `ubuntu`" | Names the `linux (ubuntu)` run |
| Nit: no step-table row for the icon check | Added |
| Nit: `set -o pipefail` was redundant under the job's `bash` default | Removed |
| Lesson proposed | Added to the [review lessons](../review-lessons.md#ci-and-tooling) |

## Verification

Locally: `python3 tools/check_docs.py` passes and `actionlint` (from PyPI's `actionlint-py`) reports nothing on `ci.yml`. The Ubuntu entry's install command is the one checked with `apt-cache policy` on Ubuntu 24.04 for [Linux beyond Arch](2026-11-22-linux-beyond-arch.md#verification), plus `git` and `ca-certificates`. No container runtime is available in this session, so the containers run first in this pull request's CI.

## Pull request

Title: `build(ci): one Linux job with a distribution matrix, Ubuntu required`

**Summary.** The Arch and Ubuntu jobs ran the same steps twice. They become one `linux` job with a matrix entry per distribution, each in its own container, and the required `checks` status passes only when every entry passed, so Ubuntu is required (the user's choice of 2026-11-23).

**Changes.**
- CI: job `linux` with entries `arch` (the pinned reference: format, docs, icon, warnings as errors) and `ubuntu` (an `ubuntu:24.04` container, warnings as annotations); `checks` waits for it.
- Docs: testing's CI section and step table, the *Tested* column of development, the roadmap item *Linux support follow-ups* (only the Fedora check is left), the open points of *Linux beyond Arch*.

**Design.** `docs/design/2026-11-23-ci-linux-matrix.md`.

**Verification.** Docs check and `actionlint` locally; the containers run first in this pull request's CI, which is the Full level.

**Review notes.** Branch protection needs no change: the required status is still `CI / checks`. The old `CI / ubuntu` status disappears.

**Dependencies and merge order.** Independent of other pull requests; Kinetra-Lab#78 makes the same change in Lab. Changes CI, so it waits for the user.

**Follow-up.** A Fedora entry once its package names are checked.

## Open points

- A Fedora entry waits for the Fedora command to be checked (roadmap *Linux support follow-ups*).
