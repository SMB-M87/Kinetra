# Local verification runs only graphics

Status: implemented · Branch: `docs/verify-levels` · Related: [testing](../testing.md#full), [verify skill](../../.agents/skills/verify/SKILL.md)

The Full level ran `ci` and `headless` locally, and in Lab also `windows`, before every push. The pull request's CI runs exactly those again on every push, and Lab's CI once more after the merge. Locally, only the graphics run adds something CI cannot do. This change splits the Full level: graphics locally, the rest in the pull request's CI.

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

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem

- In the include-layout round, the local Full level took about 15 to 20 minutes per push: `ci`, `headless` and `graphics`, plus the Lab `windows` cross build with tests through WSL interop (481 s of tests alone).
- CI runs `headless` and `ci` (and in Lab a native Windows build) on every push to the pull request; only the graphics cases are compiled there and not run, because there is no GPU.

## Goals and non-goals

- Goal: locally only what CI cannot check: the graphics workflow and Lab's smoke runs.
- Goal: a change counts as verified only when the pull request's checks are green as well.
- Non-goal: changing CI, the presets or the Docs and Targeted levels.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Local Full level | `cmake --workflow --preset graphics`, in Lab plus smoke runs | User's choice ("kan je dan niet gewoon alleen de graphics draaien en de pipeline zijn werk laten doen?"); the graphics preset builds everything and runs every suite |
| `ci`, `headless`, `windows` locally | Only to reproduce a CI failure | They duplicate the pull request's CI |
| When verified | Local graphics passed and the pull request's checks are green | A pushed change with pending checks is reported as pending, not verified |

## Design

[testing.md](../testing.md#full) states the split; `AGENTS.md`'s level table, the `verify` skill's Full step and the `feature` skill's push step follow it. Lab's `AGENTS.md` changes on its own branch.

## Affected areas

[docs/testing.md](../testing.md), [AGENTS.md](../../AGENTS.md), [verify skill](../../.agents/skills/verify/SKILL.md), [feature skill](../../.agents/skills/feature/SKILL.md).

## Test plan

Docs level: `python3 tools/check_docs.py`.

## Review

The changed sections were read against CI's steps: format, docs, icon check, `headless`, `ci` and the consumer are all in [ci.yml](../../.github/workflows/ci.yml), so nothing the old local level checked is lost.

## Verification

`python3 tools/check_docs.py`: all links resolve.

## Pull request

**docs(testing): run only graphics locally and leave the rest to CI**

The Full level now runs `cmake --workflow --preset graphics` locally (plus Lab's smoke runs) and relies on the pull request's CI for format, docs, `headless`, `ci`, the consumer and Windows. A change is verified when both pass. Updated: testing.md, AGENTS.md, the verify and feature skills. Docs only.
