# CI image measured: a shorter mirror budget instead

Status: implemented · Branch: `build/ci-mirror-budget`, and in Kinetra-Lab `ccr-55c5e0d8-yz2ai8` · Related: [CI mirror time budget](2026-11-23-ci-mirror-time-budget.md), [testing, continuous integration](../testing.md#continuous-integration)

The roadmap item *CI image with the toolchain built in*, in Kinetra and in Kinetra-Lab, asked whether a prebuilt Ubuntu image would shorten the CI's setup and make it less dependent on the package mirrors. The measurement of 2026-11-26 shows that an image would save only a few seconds per run. The one slow case is a stalled Azure mirror, which the install step now waits out for five minutes. The user chose on 2026-11-26 to build no image. Instead the mirror's budget goes from five minutes to 90 seconds, in both `ci.yml` files and in Kinetra-Lab's `release.yml`, and both roadmap items close.

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

The measurement covers the step timings of every Ubuntu run since the Ubuntu CI merged: Kinetra's `ci.yml` runs 196 to 198 and Kinetra-Lab's runs 278 to 283, which ran on 2026-11-24 to 26 (GitHub's job API, `started_at` and `completed_at` per step), plus the apt lines in the logs of three of them.

| Job | Install step | Formatter install | Whole job |
| --- | --- | --- | --- |
| Kinetra `linux` (3 runs) | 17–26 s | 2–3 s | 66–80 s |
| Kinetra-Lab `linux` (6 runs) | 17–40 s, and once 324 s | 2–4 s | 4:10–5:48, and once 10:18 |
| Kinetra-Lab `windows`, in parallel with `linux` | MSYS2 39–76 s | | 2:45–4:51 |
| Kinetra-Lab release `package` (run 80) | 16 s | | 1:28 |

- **The mirror's part is short.** The apt lines show that the index update and the download of the archives take about 10 s: in Kinetra's run 198, 189 MB arrived from `azure.archive.ubuntu.com` in 4 s. In Kinetra-Lab's slowest normal run, 283, the update started at 12:54:41 and the downloads ended at 12:54:52, 190 MB in 7 s. The rest of the step is dpkg unpacking and configuring about 250 packages, 724 MB installed.
- **An image would save little.** It would hold the same 724 MB on top of `ubuntu:24.04`, and the runner would have to pull and unpack it on each run, since GitHub's hosted runners keep no image between jobs. Unpacking takes the place of dpkg's unpacking. The estimated net gain is 5 to 15 s per run. In Kinetra-Lab, the `windows` job runs at the same time as `linux` and takes about as long, so the time until `checks` reports would change by less than that. The release `package` job runs only after a merge, and nobody waits for it; its install took 16 s of 88.
- **The slow case is the mirror's budget.** In Kinetra-Lab's run 280 (job 139901309169, 2026-11-26), the mirror delivered the indexes in 1 s. The download of `libc6`, the first package, then stalled at 12:13:28. `timeout -k 10 300` ended the attempt at 12:18:24 with the *Ubuntu mirror not used* warning, and the fallback to `archive.ubuntu.com` fetched the same 187 MB in 7 s; the step took 5:24 in all. That is one of the nine runs measured. The [2026-11-23 design](2026-11-23-ci-mirror-time-budget.md#decisions) chose five minutes to leave the fallback ten of the step's 15, because the fallback took two to three minutes before the mirror existed. It took 24 s in run 280.
- An image would remove the mirror from the pull request's path. It would replace it with a build workflow per repository, a registry, permissions to publish packages, a schedule for Ubuntu's security updates and for the pinned formatter, and a way for a pull request that changes the package list to test its own image.

## Goals and non-goals

- Goal: a stalled mirror costs about two minutes of a run, not five and a half, and the fallback keeps its time to install.
- Goal: both roadmap items close with the measurement as their record.
- Non-goal: a prebuilt image, or caching apt's archives with `actions/cache`. Both would save seconds, which the measurement does not justify. If the setup does become slow, the measurement above is the baseline.
- Non-goal: apt's own timeouts (`Acquire::http::Timeout`, `Acquire::Retries`). The 2026-11-23 design showed that only `timeout` bounds every way of not answering. A shorter per-read timeout would only add a retry within the 90 s, and the fallback after a stall takes about 25 s.
- Non-goal: the budgets of Kinetra's `tools/cloud_setup.sh`, two and five minutes. A cloud session reaches the archive through its own proxy and has no fallback mirror, so the CI numbers say nothing about it.
- Non-goal: the formatter install (2–4 s) and the MSYS2 install of Kinetra-Lab's `windows` job, which does not use the Ubuntu mirror.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Image or not | No image and no apt cache; the mirror's budget is shortened instead | The measurement above: 5 to 15 s per run against a build workflow, a registry and an update schedule in two repositories; the slow case is the budget, not the download. The user's choice of 2026-11-26, from four options: no image with a shorter budget, a prebuilt GHCR image, an apt cache, or only recording the measurement |
| The mirror's budget | `timeout -k 10 90`, around the mirror's `apt-get update` and `apt-get install --download-only` as before | The mirror's part took about 10 s in every run measured, 11 s at most, so 90 s leaves eight times that. A slow but working mirror at 3 MB/s would still deliver 190 MB in about 65 s. The worst case becomes 90 s plus the fallback's 25 s, and the fallback keeps more than 13 of the step's 15 minutes |
| Where | Kinetra's `ci.yml`, Kinetra-Lab's `ci.yml` and `release.yml`, with their warnings and comments, and [testing](../testing.md#continuous-integration) | The three install steps share one shape and one budget; the release workflow's says "the same Azure mirror and fallback as the pull request build" |
| The roadmap items | Removed from both roadmaps; this document is the record | The item's question, measure and decide, is answered. Kinetra-Lab's item also asked about the Windows packaging job: its install took 16 s of a job nobody waits for |

## Design

In each of the three install steps only the number and the words that state it change:

```bash
if ! { timeout -k 10 90 bash -c 'apt-get update -o APT::Update::Error-Mode=any && apt-get install -y --no-install-recommends --download-only $packages' && apt_install; }; then
  echo "::warning title=Ubuntu mirror not used::The Azure mirror of the Ubuntu archive failed or did not deliver the indexes and packages within 90 seconds, so this run installs from the image's own sources."
```

The comment above it says "90 seconds of the step's 15 minutes" and cites the measured 10 s. Nothing else in the step changes: the install from the downloaded packages stays unbounded, and the fallback's update and install keep no budget of their own.

## Affected areas

- Kinetra: `.github/workflows/ci.yml` (the install step's budget, warning and comment), [testing](../testing.md#continuous-integration) (the mirror's share of the step), the [roadmap](../roadmap.md) (the item removed), this document and the [design index](README.md).
- Kinetra-Lab: `.github/workflows/ci.yml` and `.github/workflows/release.yml` (the same step), its roadmap (the item removed), and the pointer to this branch.
- No build, test or source file changes.

## Test plan

A workflow has no test suite; its checks are local runs of the step and the pull requests' own CI runs.

| Behavior | Check | Level |
| --- | --- | --- |
| Mirror reachable: everything from the Azure mirror, well within 90 s | Run Kinetra-Lab's step from the committed `ci.yml` as root in a `chroot` of `ubuntu:24.04`, as the 2026-11-23 design did: exit 0, no warning, the time until the downloads end recorded | Local |
| Mirror stalls a download: the fallback starts after 90 s | The same, with the mirror's host sent to a local server that passes `/ubuntu/dists/` to the real mirror and never answers `/ubuntu/pool/`: the warning after about 90 s, the fallback, exit 0 | Local |
| The three steps carry the same budget | `grep` for `timeout -k 10` in the three workflows; Python's YAML parser loads them | Local |
| The Ubuntu run still uses the mirror on GitHub's runners | `linux` in both pull requests: no warning, green | Full (CI) |
| Docs | `check_docs.py` and `check_copies.py` with the changed terms, in both repositories | Docs |

## Review

- Own review of both diffs: only the number, the warning's words and the comment change; the three steps' `run:` blocks still differ only in their package lists and comments (compared with Python's YAML parser and `difflib`). A slow but working mirror, under 2 MB/s, now goes to the fallback, which is faster then anyway.
- Copies: `check_copies.py` in Kinetra-Lab, with this branch in `external/Kinetra`, for `five minutes`, `five of the`, `mirror's five`, `timeout -k 10 300`, `CI image`, `toolchain built in` and `prebuilt`. The hits that remain are the design index's rows, which are history, and `tools/README.md` on `cloud_setup.sh`'s own budgets, a non-goal above. The [2026-11-23 design](2026-11-23-ci-mirror-time-budget.md) got a line that points here.

## Verification

The step's `run:` came from Kinetra-Lab's committed `ci.yml`, read with Python's YAML parser. It ran as GitHub runs it (`bash --noprofile --norc -eo pipefail`), as root, under `timeout 900`, in a `chroot` of Ubuntu's `ubuntu-base-24.04.5-base-amd64` root file system, whose `ubuntu.sources` lists the same `archive` and `security` URIs as the `ubuntu:24.04` image.

| Run | Result |
| --- | --- |
| Mirror reachable | Exit 0 after 35 s, no warning; all 292 `Get:` lines from `azure.archive.ubuntu.com`. The update fetched 35.8 MB in 4 s and the downloads 194 MB in 12 s, through the session's network; the install then reported *Need to get 0 B/194 MB* |
| Mirror serves the indexes and stalls the packages (a local server on `127.0.0.2:80`, which the root's `/etc/hosts` names for the mirror; it passes `/ubuntu/dists/` to the real mirror and never answers `/ubuntu/pool/`) | The indexes from the mirror, then the first package stalled; the *Ubuntu mirror not used* warning when the 90 s ended; the fallback fetched everything from `archive.ubuntu.com` and `security.ubuntu.com` (194 MB in 38 s) and exit 0 after 150 s, where the same case took 332 s with the five-minute budget. No apt process was left |

- The three workflows load with Python's YAML parser, and each install step holds `timeout -k 10 90`.
- `python3 tools/check_docs.py` in Kinetra and `python3 external/Kinetra/tools/check_docs.py` in Kinetra-Lab: all links resolve.
- Full level: the pull requests' CI, recorded in their descriptions.

## Pull request

Title: `build(ci): measure the CI setup and shorten the mirror's budget to 90 seconds`

Summary: the roadmap item *CI image with the toolchain built in* asked whether a prebuilt image would shorten the CI's setup. It would save about 5 to 15 s per run (see [Problem](#problem)), so the user chose on 2026-11-26 to build none. The one slow case measured, a stalled Azure mirror that cost run 280 five and a half minutes, is fixed instead: the mirror's index update and downloads get 90 seconds instead of five minutes, in Kinetra's `ci.yml` and in Kinetra-Lab's `ci.yml` and `release.yml`. [testing](../testing.md#continuous-integration) follows, and the roadmap item is removed from both roadmaps. Merge order: this pull request first, with a merge commit, then the Kinetra-Lab pull request, which moves the pointer to this branch.

## Open points

None.
