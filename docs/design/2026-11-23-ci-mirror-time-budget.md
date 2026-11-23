# CI mirror time budget and the remaining time limits

Status: accepted · Branch: `fix/ci-mirror-time-budget`, and in Kinetra-Lab `ccr-37e580f0-jg7n7e` (Kinetra-Lab #98) · Related: [CI fails fast on a stalled install](2026-11-23-ci-stalled-install.md), [testing, continuous integration](../testing.md#continuous-integration)

A follow-up to [CI fails fast on a stalled install](2026-11-23-ci-stalled-install.md) (#42 and Kinetra-Lab #98), from the agent review of Kinetra-Lab #98 and the user's decisions of 2026-11-23. The Ubuntu install step falls back to the image's sources when the Azure mirror fails, but a mirror that connects and then never answers can use up the step's 15 minutes before the fallback starts. The mirror's index update now gets a time budget of its own. Kinetra-Lab's `windows` install step and its release workflow get the time limits that #42 left out, and the step warns when its pattern misses a URI.

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

- The mirror's attempt has no time budget of its own. `.github/workflows/ci.yml:168` (Kinetra) and Kinetra-Lab's `ci.yml:196` run `apt-get update` against `azure.archive.ubuntu.com` inside the step's 15 minutes, and the fallback starts only when it fails. #42 tested a refused connection and a failing index, which fail within seconds. Measured with #42's step, as root in a `chroot` of `ubuntu:24.04`, with `azure.archive.ubuntu.com` sent to an address that does not answer:
  - packets dropped (a firewall `DROP` for `10.255.255.1`): apt's connection timed out after 30 s, its retries failed at once, and the fallback started after 37 s;
  - TCP accepted and no byte sent, a hung load balancer (a local listener on `127.0.0.2:80`): see [Verification](#verification).
- #42's design leaves apt's own retry and timeout settings out ([non-goal](2026-11-23-ci-stalled-install.md#goals-and-non-goals)), because the stalled run was slow, not stuck; that reason covers the slow archive, not a mirror that does not answer.
- Kinetra-Lab's `ci.yml` header says that each install step has a time limit, a sentence copied from Kinetra's header, but its `windows` job's `msys2/setup-msys2@v2` step, which updates MSYS2 and installs six packages from MSYS2's mirrors, has none; a stall there runs until the job's 45 minutes.
- Kinetra-Lab's `release.yml` has no time limit on any job. #42 left it out because "the release workflow waits for the pinned toolchain on purpose", citing the [2026-11-21 design](2026-11-21-ci-mirror-fallback.md#goals-and-non-goals); Kinetra-Lab c08ac34 (2026-11-22) had already made its `package` job fall back to the image's mirrors, so it no longer waits, and its Arch install can stall like CI's.
- When the step's `sed` matches no `URIs:` line, for example when a later image writes `https://` URIs, the run installs from `archive.ubuntu.com` without a warning; only the printed URIs show it.
- The step's shell function `install` hides coreutils' `install` for the rest of the step.
- Stale comments in the workflows: Kinetra `ci.yml:125` says the limit "leaves room for a build without cache", but CI restores no compiler cache, so every build is one; Kinetra `ci.yml:150` "Usually two minutes" and Kinetra-Lab `ci.yml:177` "Usually three minutes" date from before the mirror, and #42's and #98's installs took 23 s and 28 s; Kinetra-Lab `ci.yml:152` "about 11 minutes", where #98's Ubuntu run took 8.
- [testing](../testing.md#continuous-integration) says "Every job has a time limit" right before the paragraph on `lab-pointer.yml`, whose job has none, and that CI falls back "when the mirror fails or lacks an index".

## Goals and non-goals

- Goal: a mirror that does not answer sends the run to the fallback well within the step's 15 minutes, with the *Ubuntu mirror not used* warning.
- Goal: every install step and every job of both `ci.yml` files and of Kinetra-Lab's `release.yml` has a time limit, and the comments and pages that say so are true.
- Goal: a run that does not use the mirror says so, whether the mirror failed or the step's pattern missed a URI.
- Non-goal: a time budget on `apt-get install`. Killing it can leave dpkg half-configured. A mirror that answers the index update and then stalls the downloads still runs into the step's limit, as #42 designed for a slow mirror.
- Non-goal: `lab-pointer.yml`; it installs nothing and is not part of `ci.yml`'s time limits.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| The mirror's budget | `timeout -k 10 300` around the mirror's `apt-get update` only; the fallback's update and both installs keep no budget of their own | Reverses #42's non-goal on apt's settings. The update fetches only the indexes, a few seconds from the mirror; five minutes leave the fallback ten, and the fallback took two to three minutes from `archive.ubuntu.com` before the mirror. `timeout` bounds every way of not answering, dropped packets, a silent connection or a trickle, where apt's `Acquire::http::Timeout` bounds only the wait for each read. `-k 10` kills apt if it ignores the TERM. Never around `apt-get install`, which can leave dpkg half-configured. Settled with the user on 2026-11-23 |
| Kinetra-Lab's release workflow | `timeout-minutes: 15` on the `package` job's install step; 30 minutes for `package`, 10 for `changes` and `publish` | Reverses #42's non-goal, whose reason no longer held after Kinetra-Lab c08ac34. The user asked for the install step and the `package` job, in line with `ci.yml`, where every job has a limit; `changes` and `publish` take seconds. `package` took two minutes on 2026-11-23 (run 45177035588: install 27 s, cross build 64 s); 30 minutes leave the install its 15 and the build as much again. Settled with the user on 2026-11-23 |
| Kinetra-Lab's MSYS2 install | `timeout-minutes: 15` on the `msys2/setup-msys2@v2` step | Makes the header's "each install step" true for Lab, as for Kinetra; a step limit also works on a `uses:` step. The step took one minute in #98's run. From the agent review of Kinetra-Lab #98 |
| A URI the pattern misses | After the `sed`, warn *Ubuntu mirror not used* when any `URIs:` line is not the Azure mirror; the run goes on | The fallback's warning covers only a failing mirror. Checking every line, not only one, also catches an image that adds a source. From the agent review of Kinetra-Lab #98 |
| The function's name | `apt_install`, which runs only `apt-get install`; each update is written out | It no longer hides coreutils' `install`, and the mirror's update, the only one with a budget, stands on its own line |

## Design

The Ubuntu install step becomes, in both repositories (Kinetra-Lab's package list adds `nlohmann-json3-dev`):

```bash
sources=/etc/apt/sources.list.d/ubuntu.sources
cp "$sources" /etc/apt/ubuntu.sources.image
sed -i -E 's#^URIs: http://(archive|security)\.ubuntu\.com/ubuntu/?$#URIs: http://azure.archive.ubuntu.com/ubuntu/#' "$sources"
grep '^URIs:' "$sources"
if grep -q -v -x 'URIs: http://azure\.archive\.ubuntu\.com/ubuntu/' <<< "$(grep '^URIs:' "$sources")"; then
  echo "::warning title=Ubuntu mirror not used::…"
fi
apt_install() { apt-get install -y --no-install-recommends $packages; }
if ! { timeout -k 10 300 apt-get update -o APT::Update::Error-Mode=any && apt_install; }; then
  echo "::warning title=Ubuntu mirror not used::…"
  cp /etc/apt/ubuntu.sources.image "$sources"
  apt-get update
  apt_install
fi
```

The fallback's update and install are separate lines, so the step's `-e` fails the run on either. A here-string feeds the check, as in the `changes` job, so no SIGPIPE can decide it. In Kinetra-Lab, `release.yml` gets `timeout-minutes` on its three jobs and its install step, and `ci.yml` on the `setup-msys2` step. The comments that state durations take #42's and #98's CI runs.

## Affected areas

- Kinetra: `.github/workflows/ci.yml` (the Ubuntu install step, its comment, the header, the `linux` job's comment), [testing](../testing.md#continuous-integration) (the fallback and the time limits), [review lessons](../review-lessons.md#docs-and-links), this document, the [design index](README.md), and a pointer from the two reversed non-goals of [CI fails fast on a stalled install](2026-11-23-ci-stalled-install.md#goals-and-non-goals).
- Kinetra-Lab, on #98's branch: `.github/workflows/ci.yml` (the same step, the `setup-msys2` limit, the header and comments), `.github/workflows/release.yml` (time limits) and its review lessons.
- No build, test or source file changes.

## Test plan

A workflow has no test suite; its checks are local runs of the step and the pull requests' own CI runs.

| Behavior | Check | Level |
| --- | --- | --- |
| Mirror reachable: every package from the Azure mirror | Run Kinetra-Lab's step as root in a `chroot` of `ubuntu:24.04`, as #42 did: exit 0, the Azure URIs, every `Get:` from `azure.archive.ubuntu.com`, no warning | Local |
| Mirror refused: the warning and the fallback | The same with `azure.archive.ubuntu.com` sent to `127.0.0.1` in the root's `/etc/hosts`: the warning, the image's URIs restored, exit 0 | Local |
| Mirror silent: the fallback starts well within 15 minutes | The same with the host sent to `10.255.255.1`, dropped by a firewall rule, because the session's network answers every other address; and to a local listener that accepts and never answers. The warning, the fallback, exit 0; the time until the warning recorded | Local |
| The pattern misses a URI: a warning | The check on its own, fed the URIs of an image that writes `https://` | Local |
| Every job and install step has a limit | Load both `ci.yml` files and Kinetra-Lab's `release.yml` with Python's YAML parser and list every `timeout-minutes` | Local |
| The Ubuntu run still uses the mirror on GitHub's runners | `linux (ubuntu)` in both pull requests: the Azure URIs, no warning, green | Full (CI) |
| Docs | `python3 tools/check_docs.py`; `check_copies.py` with the changed terms, in both repositories | Docs |

The release workflow runs only after a merge on `main` or a tag, so its limits are checked by the parser only.

## Review

Not yet.

## Verification

Not yet.

## Pull request

Title: `fix(ci): give the Ubuntu mirror a time budget of its own`.

Not yet complete.

## Open points

None yet.
