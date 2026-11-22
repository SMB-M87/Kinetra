# Roadmap priorities

Status: implemented · Branch: `docs/roadmap-priorities` (stacked on `docs/stacked-pull-requests`) · Related: [roadmap](../roadmap.md), [stacked pull requests](2026-11-21-stacked-pull-requests.md), [Kinetra-Lab design](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-21-roadmap-priorities.md)

The user closed Lab's quality roadmap as a plan: everything still open belongs in the roadmaps, in order of priority, so it is clear to everyone what comes first. They also brought new wishes, several of which need the engine. This change brings Kinetra's roadmap in line with Lab's: stale items out, no more references to the quality roadmap, and the engine half of every new wish as an item.

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

- *Demo's contact tests in Kinetra* is still under Next, but the suite merged in #19 and Lab dropped its copy in Kinetra-Lab #64.
- Three places point at Lab's quality roadmap for scope: the roadmap's introduction, *Skill READMEs* and *Shared passing mechanism*, and the skills overview's open points. The user wants the roadmaps to be the only plan.
- The user's new wishes of 2026-11-21 have engine parts with no item: a skill that reviews pull requests (skills live here), an overlay that inspects one agent, Linux support beyond Artix and Arch, a critical look at performance, graphics tests that CI can run, and the question whether the docs could move to the GitHub wiki.

## Goals and non-goals

- Goal: every open engine item on this page, self-contained, in the order proposed with Lab's roadmap.
- Non-goal: changing design documents of earlier branches; they are records and keep their links.
- Non-goal: starting any item.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Order | Process and tooling first (review skill, graphics tests in CI), then the inspector overlay, then the rest | Proposed by the agent; the user confirms or changes it with Lab's order. Every later pull request profits from the first two, and the inspector is the start of the user's motion thread |
| Shared passing mechanism | From Next to Later, after Lab's stall detection and remedies | Lab first proves making way and a temporary motion fallback in i4sim; the shared mechanism then knows whether they belong in it, as the quality roadmap's rule "share only what is proven" asked |
| Linux beyond Arch | Next in Kinetra, Later in Lab | Kinetra has no golden fingerprints that depend on the compiler; Lab has five, and they fail on Ubuntu's GCC 13 |
| Wiki | An idea, with the assessment in Lab | A question of the user, not yet a wish; the agent recommends a generated mirror over a move |
| Stacked on `docs/stacked-pull-requests` | Yes | Both edit the design index and the skills overview, and Lab records one pointer that needs both |

## Design

[Roadmap](../roadmap.md): the introduction states the order rule and that the quality roadmap is closed; *Demo's contact tests in Kinetra* is removed; *Shared passing mechanism* holds its scope itself; new items *Pull request review skill*, *Graphics tests in CI*, *Agent inspector overlay*, *Linux beyond Arch*, *Engine performance* and the idea *Documentation as a site*. The skills overview's open point names the roadmap item.

## Affected areas

`docs/roadmap.md`, `.agents/skills/README.md` (open points), this document and the design index.

## Test plan

Documentation only: the Docs level (`python3 tools/check_docs.py`).

## Review

Checked every item against its source and against Lab's roadmap: each shared item exists in both, with the same title and its own part.

## Verification

- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**docs(roadmap): order the backlog and add the engine half of the new wishes**

- **Summary:** the roadmap lists the open engine work in order of priority, without references to Lab's closed quality roadmap, and gains items for the user's new wishes.
- **Changes:** roadmap (stale contact-tests item out; review skill, graphics tests in CI, inspector overlay, Linux beyond Arch, engine performance and a documentation-site idea in; shared passing to Later); skills overview open point.
- **Design:** `docs/design/2026-11-21-roadmap-priorities.md`.
- **Verification:** Docs level, green.
- **Dependencies and merge order:** top of a stack of three: `ci/archive-mirrors` (#22), `docs/stacked-pull-requests` (#20), then this, each with a merge commit. The Kinetra-Lab pull request on `ccr-cc3e854a-v10cnx` records this branch and merges last.
- **Follow-up:** the order is the agent's proposal; the user's answer may reorder it in a later commit.

## Open points

- Settled with the user on 2026-11-22: [roadmap decisions](2026-11-22-roadmap-decisions.md).
