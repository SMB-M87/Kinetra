# CI fails fast on a stalled install

Status: implemented · Branch: `fix/ci-stalled-install`, and in Kinetra-Lab `ccr-37e580f0-jg7n7e` · Related: [testing, continuous integration](../testing.md#continuous-integration), [CI falls back to current Arch packages](2026-11-21-ci-mirror-fallback.md), Kinetra-Lab #97 (the stalled run), [CI mirror time budget](2026-11-23-ci-mirror-time-budget.md) (the follow-up)

On 2026-11-23, the `linux (ubuntu)` run of Kinetra-Lab #97 spent an hour in its install step, which normally takes three minutes. It had not stopped; `archive.ubuntu.com` served the packages a few kB at a time, and nothing in CI bounds how long a job may run, apart from GitHub's six hours. This change installs Ubuntu's packages from Canonical's mirror inside Azure, where GitHub's runners run, falls back to the image's own sources when that mirror fails, and gives every CI job and both install steps a time limit, in Kinetra and in Kinetra-Lab.

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

- Kinetra-Lab run 45159449271 (#97, head `5a5f6a9`): the step *Install Ubuntu's packages* of `linux (ubuntu)` started at 10:48:46 UTC and was still running when it was cancelled at 11:48:44. In run 45150039409 on the same branch an hour earlier, the same step took three minutes (09:46:52 to 09:49:48); `linux (arch)` and `windows` of the stalled run passed in ten and six minutes.
- The log shows a slow archive, not a hung process: `Get:200 … libpcre2-dev [737 kB]` took 44 seconds, packages of 15 to 60 kB took 30 to 40 seconds, and `Ign:` lines, downloads apt gave up on and retried later, came about once a minute. After an hour apt was at package 222. The job's re-run at 11:49 passed, and its install took ten minutes.
- The `ubuntu:24.04` image reads its packages from `http://archive.ubuntu.com/ubuntu/` and `http://security.ubuntu.com/ubuntu/`, in `/etc/apt/sources.list.d/ubuntu.sources`; its `/etc/apt/sources.list` holds no entry (read from the image's layer `sha256:8e8df940…`, amd64, on 2026-11-23).
- No job in either `ci.yml` sets `timeout-minutes`, so GitHub ends a stalled job only after 360 minutes, and the required `checks` waits for it. Kinetra's `ci.yml` has the same install step (`Install Ubuntu's packages`) and the same gap.
- Measured from a cloud session at about 11:55 UTC the same day, three requests each for `dists/noble-updates/InRelease`: `archive.ubuntu.com` answered in 6.8 s, 10.0 s (HTTP 504) and 0.8 s; `azure.archive.ubuntu.com` in 0.28 s, 0.16 s and 0.10 s, and it serves `noble`, `noble-updates` and `noble-security`.

## Goals and non-goals

- Goal: a stalled install, or any stalled job, fails within a bounded time, so the author can re-run it instead of waiting hours.
- Goal: the Ubuntu run downloads from a mirror close to GitHub's runners, and still installs Ubuntu's own packages.
- Goal: when that mirror fails, the run still installs from the image's own sources, and says so.
- Non-goal: pinning Ubuntu's packages or a prebuilt CI image; the Kinetra roadmap item *CI image with the pinned toolchain* covers the Arch side.
- Non-goal: apt's own retry and timeout settings. The stalled run was slow, not stuck on one connection; the time limits bound both cases. *Reversed on 2026-11-23 by [CI mirror time budget](2026-11-23-ci-mirror-time-budget.md): a mirror that connects and never answers used up the step's limit before the fallback started, so the mirror's update and downloads now have five minutes of their own.*
- Non-goal: `lab-pointer.yml` in Kinetra and `release.yml` in Kinetra-Lab. Neither installs from `archive.ubuntu.com`, and the release workflow waits for the pinned toolchain on purpose ([CI falls back to current Arch packages](2026-11-21-ci-mirror-fallback.md#goals-and-non-goals)). *Reversed for `release.yml` on 2026-11-23 by [CI mirror time budget](2026-11-23-ci-mirror-time-budget.md): since Kinetra-Lab c08ac34 it no longer waits, and it now has the same limits.*

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Scope | Kinetra and Kinetra-Lab, with this one design document in Kinetra | Both `ci.yml` files carry the same install step and no time limits, and [testing](../testing.md#continuous-integration) holds the shared CI design. Settled with the user on 2026-11-23 |
| Ubuntu mirror | `http://azure.archive.ubuntu.com/ubuntu/` for both URIs of the image's `ubuntu.sources` | Canonical's mirror inside Azure, where GitHub's hosted runners run. The host was checked with requests first ([review lesson](../review-lessons.md#ci-and-tooling)): it answered 30 to 60 times faster than `archive.ubuntu.com` (see [Problem](#problem)). Settled with the user |
| When the mirror fails | Restore the image's `ubuntu.sources`, install again, and warn *Ubuntu mirror not used* | The pattern of the Arch step, which falls back to the image's mirror list. The fallback names no host of its own, so it cannot be wrong the way a remembered host can. Settled with the user |
| A partial index | The mirror's `apt-get update` runs with `-o APT::Update::Error-Mode=any`; the fallback's without it, as today | `apt-get update` exits 0 when an index fails to download, with only a warning, and `apt-get install` then installs from what it has: older versions, when `noble-updates` is the missing index. Measured in a root file system of the image: one failing index exits 0 by default and 100 with the option, and working sources exit 0 with it. The fallback keeps today's behavior, so this change makes no run fail that passes today |
| Time limits | Every job of `ci.yml`: 45 minutes for `linux` and Kinetra-Lab's `windows`, 10 for `changes` and `checks`; 15 minutes on both install steps (Arch and Ubuntu) | The slowest job took 11 minutes (Kinetra-Lab's `linux (ubuntu)`), the Ubuntu install 3 and the Arch install under one; `changes` and `checks` take seconds. The limits leave three to four times that, and a stalled install fails in 15 minutes instead of 360. Settled with the user |

## Design

The Ubuntu install step becomes:

```bash
packages="build-essential cmake ninja-build ccache libsdl2-dev libgl-dev libegl-dev libgl1-mesa-dri libegl-mesa0 git ca-certificates"
sources=/etc/apt/sources.list.d/ubuntu.sources
cp "$sources" /etc/apt/ubuntu.sources.image
sed -i -E 's#^URIs: http://(archive|security)\.ubuntu\.com/ubuntu/?$#URIs: http://azure.archive.ubuntu.com/ubuntu/#' "$sources"
grep '^URIs:' "$sources"
install() { apt-get update "$@" && apt-get install -y --no-install-recommends $packages; }
if ! install -o APT::Update::Error-Mode=any; then
  echo "::warning title=Ubuntu mirror not used::…"
  cp /etc/apt/ubuntu.sources.image "$sources"
  install
fi
```

Kinetra-Lab's package list adds `nlohmann-json3-dev`, as today. `Error-Mode=any` sends a mirror that lacks one index to the fallback too. The saved copy goes to `/etc/apt/`, not to `sources.list.d/`, where apt would report a file with an unknown extension. `grep` prints the URIs the run uses, so a log shows whether the mirror was used. Both install steps get `timeout-minutes: 15`, and every job its own `timeout-minutes`. A job or step that runs out of time fails, so `checks` fails too: it requires every `linux` run to have passed.

The fallback runs when the mirror fails, not when it is slow: a slow mirror, like a slow fallback, runs into the step's time limit, and the run fails after 15 minutes.

## Affected areas

- Kinetra: `.github/workflows/ci.yml` (the Ubuntu install step, `timeout-minutes` on every job and on both install steps, the header comment), [testing](../testing.md#continuous-integration) (the `ubuntu` entry and the time limits), this document and the [design index](README.md).
- Kinetra-Lab: `.github/workflows/ci.yml` (the same changes for its jobs, `windows` included, and the header comment).
- No build, test or source file changes.

## Test plan

A workflow has no test suite; its checks are a local run of the new step and the pull requests' own CI runs.

| Behavior | Check | Level |
| --- | --- | --- |
| The step installs every package from the Azure mirror | Run the step's script as root in a root file system of `ubuntu:24.04` (the image's layer, unpacked, in a `chroot`), Kinetra-Lab's package list; it exits 0, prints the Azure URIs, and apt's `Get:` lines name `azure.archive.ubuntu.com` | Local |
| When the mirror fails, the step restores the image's sources and installs from them, with the warning | The same run in a fresh root file system whose `/etc/hosts` sends `azure.archive.ubuntu.com` to `127.0.0.1`; it prints the warning, restores the `archive.ubuntu.com` URIs and exits 0 | Local |
| Both workflows parse, and every job and both install steps carry a time limit | Load both files with Python's YAML parser and list the `timeout-minutes` of every job and step | Local |
| The Ubuntu run uses the mirror on GitHub's runners and passes | `linux (ubuntu)` in both pull requests: the step log names `azure.archive.ubuntu.com`, no *Ubuntu mirror not used* warning, the run is green | Full (CI) |
| Docs | `python3 tools/check_docs.py` in Kinetra; in Kinetra-Lab `python3 external/Kinetra/tools/check_docs.py` and `check_copies.py` | Docs |

A run that reaches a time limit is not provoked: that would take a commit that stalls a step on purpose, and GitHub documents what `timeout-minutes` does.

## Review

- `/code-review` (low) on the Lab diff, which equals Kinetra's but for the package list and the `windows` job: no findings.
- Own review: the first version ran `apt-get update` without `Error-Mode=any`. A test showed that it exits 0 when one index fails, so a half-working mirror would have installed from a partial list without falling back; the option was added for the mirror's attempt.
- Copies: `check_copies.py` with `Install Ubuntu's packages`, `archive.ubuntu.com`, `apt-get`, `timeout-minutes`, `six hours` and `image tag follows`, in both repositories. Every hit is a line this change wrote, apart from Lab's roadmap item on cloud sessions, which is about a different `apt-get`.

## Verification

Each step run took the step's `run:` from `ci.yml` with Python's YAML parser and ran it as GitHub does (`bash --noprofile --norc -eo pipefail`), as root, in a `chroot` of the unpacked `ubuntu:24.04` layer (amd64, `sha256:8e8df940…`):

| Run | Result |
| --- | --- |
| Kinetra's step, mirror reachable | Exit 0 after 41 s; every `Get:` from `azure.archive.ubuntu.com`; gcc, cmake, ninja, ccache, git and `libsdl2-dev` installed |
| Kinetra-Lab's step, mirror reachable | Exit 0 after 40 s; the same, plus `nlohmann-json3-dev` 3.11.3 |
| Kinetra-Lab's step, `azure.archive.ubuntu.com` sent to `127.0.0.1` | The *Ubuntu mirror not used* warning, the image's URIs restored, every `Get:` from `archive.ubuntu.com`, exit 0 after 34 s |
| Kinetra's step, one extra source refusing connections (a partial index) | The mirror's update fails (`E: Some index files failed to download`), the warning, the fallback installs with a `W:` for that source, exit 0 after 41 s |
| `apt-get update` with one failing index | Exit 0 by default, 100 with `-o APT::Update::Error-Mode=any`; working sources exit 0 with it |

- Both workflows load with Python's YAML parser; every job has `timeout-minutes` (10, 45, 10, and 45 for Lab's `windows`), and so do both install steps (15).
- `python3 tools/check_docs.py` in Kinetra and `python3 external/Kinetra/tools/check_docs.py` in Kinetra-Lab: all links resolve.
- Full level: CI passed on both pull requests, Kinetra #42 (run 45172033042, head `0e03cb4`) and Kinetra-Lab #98 (run 45172060090, head `013d987`). Their *Install Ubuntu's packages* took 23 s and 28 s, against two to three minutes before and ten minutes in #97's re-run that day; the logs show the Azure URIs, every `Get:` (267 and 268) from `azure.archive.ubuntu.com`, no `E:` or `W:` line and no *Ubuntu mirror not used* warning.

## Pull request

Title: `fix(ci): bound every job and install Ubuntu's packages from the Azure mirror`, in both repositories.

Summary: Kinetra-Lab #97's Ubuntu install ran for an hour on a slow `archive.ubuntu.com`, and no CI job has a time limit below GitHub's six hours. Every job and both install steps now have one, and the Ubuntu step installs from Canonical's mirror inside Azure, falling back to the image's sources with a warning. Changes, verification and review as in the sections above. Merge order: Kinetra first, then Kinetra-Lab; neither depends on the other's code, and the Lab pull request does not move the pointer. Both change CI, so both wait for the user.

## Open points

None yet.
