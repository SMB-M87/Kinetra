# CI falls back to the archive's mirrors

Status: implemented · Branch: `ci/archive-mirrors` · Related: [testing, continuous integration](../testing.md#continuous-integration), `docs/stacked-pull-requests` (#20, stacked on this branch)

CI installs its pinned toolchain from one host, `archive.archlinux.org`. On 2026-11-21 from 18:11 UTC that host did not answer, and every pull request in both repositories failed in its first step. This change lists the archive's mirrors after it, so pacman fetches the same dated snapshot elsewhere when one host is down.

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

- `ci.yml` writes one `Server` line, for `archive.archlinux.org`. When that host does not answer, `pacman -Syyuu` stops: `failed retrieving file 'core.db' from archive.archlinux.org : Connection timed out after 10000 milliseconds`.
- On 2026-11-21 this failed Kinetra #20 and #21 at 18:11 and again on their re-runs at 18:14, and Kinetra-Lab #65 at 18:12 and 18:34. With the same snapshot CI passed at 12:11 that day.

## Goals and non-goals

- Goal: CI gets the same pinned packages when one host is down.
- Non-goal: a package cache. It would also survive an outage of all hosts, but it is only filled while the archive works, and it adds cache keys to maintain. Worth it only if outages of every mirror happen.
- Non-goal: Kinetra-Lab's workflows. They get the same change on their own branch, which records this commit.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Fallback | The archive's mirrors (`europe`, `america` and `asia.archive.pkgbuild.com`) after the archive itself | They serve the same dated snapshots, so the pinning stays exact. pacman tries the servers in the order of the mirror list and moves to the next on a failed download, and checks package signatures whatever the host |
| Order | The archive first | It is the authoritative host; the mirrors are used only when it fails |
| Verification | This pull request's own CI | The agent's cloud session cannot reach any of these hosts. A green run while the archive is down shows that the fallback works |

## Design

The install step writes four `Server` lines, one per host, with the same `repos/<snapshot>/$repo/os/$arch` path. The header comment and [testing](../testing.md#continuous-integration) say where the snapshot comes from.

## Affected areas

`.github/workflows/ci.yml`, `docs/testing.md`, this document and the design index.

## Test plan

- The run block parses as YAML. Run with `ARCH_SNAPSHOT=2026/11/17`, it writes the four expected lines.
- This pull request's CI: the `checks` job passes the install step while `archive.archlinux.org` is down, then runs format, docs, headless and full builds as before.

## Review

The mirror path format is the archive's own (`/repos/YYYY/MM/DD/`). If a mirror host did not exist, pacman would skip it like a failed host, so a wrong name costs nothing but a fallback.

## Verification

- `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml'))"` parses the workflow, and the run block writes the four `Server` lines.
- `python3 tools/check_docs.py`: all links resolve.
- CI on this pull request: see the pull request.

## Pull request

**ci: fall back to the Arch Linux Archive's mirrors**

- **Summary:** CI keeps working when `archive.archlinux.org` is down. Its mirrors serve the same snapshot and are listed after it.
- **Changes:** install step and header comment of `ci.yml`; one sentence in testing.
- **Design:** `docs/design/2026-11-21-archive-mirrors.md`.
- **Verification:** YAML parse and a dry run of the mirror list; this pull request's CI.
- **Dependencies and merge order:** bottom of the stack: this pull request, then #20, then #21, each with a merge commit. Kinetra-Lab #65 makes the same change to its workflows.
- **Follow-up:** none.

## Open points

- If every host is down at once, CI still fails. A package cache would cover that (non-goal above).
