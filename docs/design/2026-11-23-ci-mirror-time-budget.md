# CI mirror time budget and the remaining time limits

Status: implemented · Branch: `fix/ci-mirror-time-budget`, and in Kinetra-Lab `ccr-37e580f0-jg7n7e` (Kinetra-Lab #98) · Related: [CI fails fast on a stalled install](2026-11-23-ci-stalled-install.md), [testing, continuous integration](../testing.md#continuous-integration)

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
  - TCP accepted and no byte sent, a hung load balancer (a local listener on `127.0.0.2:80`): apt waited 60 s for each of the four indexes, in turn, and retried each; at 841 s it was at its second index's last attempt, and at 900 s the step's limit ended the run (exit 124) with no warning, no fallback and nothing installed. The session that wrote #42 measured the same case to its end: apt gave up after 967 s.
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
- Kinetra-Lab, on #98's branch: `.github/workflows/ci.yml` (the same step, the `setup-msys2` limit, the header and comments), `.github/workflows/release.yml` (time limits and its header), the [release tool's README](https://github.com/SMB-M87/Kinetra-Lab/blob/main/tools/release/README.md) (the limits) and its review lessons.
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

- `/code-review` (low) on both workflow diffs: no findings.
- Own review: a step where the mirror's update and the install ran in one function would have put the budget around the install too; the update now has its own line, and the fallback's update and install are two lines, so the step's `-e` fails the run on either.
- Copies: `check_copies.py` with `each install step`, `both install steps`, `install step`, `time limit`, `timeout-minutes`, `lacks an index`, `mirror fails`, `Ubuntu mirror not used`, `Every job has`, `six hours`, `Usually two minutes`, `Usually three minutes`, `about 11 minutes`, `without cache`, `waits for the pinned toolchain`, `setup-msys2`, `apt-get update`, `Error-Mode` and `azure.archive`, in Kinetra-Lab with this branch checked out in `external/Kinetra`. Two pages still said less than the change: [testing](../testing.md#continuous-integration) named only Kinetra-Lab's `ci.yml`, now its `release.yml` too, and Kinetra-Lab's release tool README described the install's fallback but not the limits; both changed. The other hits are lines this change wrote, the merged design documents, and Kinetra-Lab's roadmap item on cloud sessions, which is about a different `apt-get update`.
- The session that wrote #42 and #98 had started the same fixes on a Kinetra branch `fix/ci-mirror-hang`, without a pull request; it stopped and left them to this branch. Whether that branch is deleted is the user's decision.

## Verification

Each step run took the step's `run:` from the committed `ci.yml` with Python's YAML parser and ran it as GitHub does (`bash --noprofile --norc -eo pipefail`), as root, in a `chroot` of the unpacked `ubuntu:24.04` layer (amd64, `sha256:8e8df940…`, from `mirror.gcr.io` because Docker Hub answered 429), under `timeout 900`, the step's limit. The session's network answers every address, `10.255.255.1` included (HTTP 403 from its proxy), so the silent address needed a firewall rule, `iptables -A OUTPUT -d 10.255.255.1 -j DROP`, checked first: a `curl` to it timed out after 8 s.

| Run | Result |
| --- | --- |
| Kinetra-Lab's step, mirror reachable | Exit 0 after 56 s; the Azure URIs, every `Get:` (268) from `azure.archive.ubuntu.com`, no warning; gcc, cmake, ninja, ccache, git, `libsdl2-dev` and `nlohmann-json3-dev` 3.11.3 installed |
| Kinetra's step, mirror reachable | Exit 0 after 46 s; every `Get:` (267) from `azure.archive.ubuntu.com`, no warning |
| Kinetra-Lab's step, mirror refused (`127.0.0.1`) | The warning after 7 s, the image's URIs restored, every `Get:` from `archive.ubuntu.com` and `security.ubuntu.com`, exit 0 after 38 s |
| Kinetra-Lab's step, mirror silent (`10.255.255.1`, dropped) | apt's connection timed out after 30 s and its retries failed at once; the warning after 38 s, the fallback, exit 0 after 70 s. #42's step behaves the same (warning after 37 s): apt already gives up on a host it cannot connect to |
| Kinetra-Lab's step, mirror accepts and never answers (a local listener) | The warning after 300 s, when `timeout` ended the mirror's update; the fallback installed everything from the image's sources; exit 0 after 330 s. The TERM was enough: no apt process was left |
| #42's step, the same listener | No warning; the step's limit ended it after 900 s (exit 124) with nothing installed; apt was at its second index's last attempt |
| The URI check on its own, fed three `ubuntu.sources` | Today's image: no warning; URIs written with `https://`: the warning; one more source: the warning |

- The session that wrote #42 measured, in the same kind of `chroot` against a listener that never answers: `apt-get update` with apt's defaults gave up after 967 s; with `-o Acquire::http::Timeout=20 -o Acquire::Retries=1` after 326 s; under `timeout 60` it ended after 60 s (exit 124) with no apt process left and `apt-get check` clean. With sources the `sed` does not recognize (`http://ARCHIVE.ubuntu.com/ubuntu/`), its step warned and installed from them.
- The three workflows load with Python's YAML parser. Kinetra's `ci.yml`: `changes` 10, `linux` 45, `checks` 10; both install steps 15. Kinetra-Lab's `ci.yml`: the same, `windows` 45, and `msys2/setup-msys2@v2` 15. Kinetra-Lab's `release.yml`: `changes` 10, `package` 30, `publish` 10; its install step 15.
- Kinetra-Lab's release, which runs only after a merge, built locally as its [tool README](https://github.com/SMB-M87/Kinetra-Lab/blob/main/tools/release/README.md) says, on Ubuntu 24.04 with `g++-mingw-w64-x86-64-posix` 13, at #98's head `862100a`: `cmake --preset windows`, the build of the four targets and `package_windows.sh` exit 0; the workflow's DLL import check finds nothing unexpected in the four executables; the zip holds 25 files.
- `python3 tools/check_docs.py` in Kinetra and `python3 external/Kinetra/tools/check_docs.py` in Kinetra-Lab: all links resolve.
- Full level: the pull requests' CI.

## Pull request

Title: `fix(ci): give the Ubuntu mirror's index update a time budget of its own`.

Summary: #42's fallback to the image's sources started only when the Azure mirror failed. A mirror that accepts connections and never answers kept apt in its timeouts and retries past the install step's 15 minutes, so the run failed with no warning and no fallback (measured: still at the second index after 841 s). The mirror's `apt-get update` now runs under `timeout -k 10 300`; the installs stay unbounded, because a killed `apt-get install` can leave dpkg half-configured. The step also warns when its pattern misses one of the image's URIs, its function `install` became `apt_install`, and stale durations in the comments are corrected. In Kinetra-Lab, #98 carries the same step, a time limit on the `windows` job's MSYS2 install and on every job and the install step of `release.yml`. Changes, verification and review as in the sections above. Merge order: this pull request first, then Kinetra-Lab #98; neither depends on the other's code, and #98 does not move the pointer. Both change CI, so both wait for the user.

## Open points

- A mirror that answers the index update and then stalls the package downloads still ends the run at the step's 15 minutes, without the fallback: the install gets no budget (see [Goals and non-goals](#goals-and-non-goals)). A re-run is the remedy, as for any stalled install.
