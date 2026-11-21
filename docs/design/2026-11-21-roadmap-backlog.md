# Roadmap as a backlog

Status: implemented · Branch: `docs/roadmap-backlog` · Related: [roadmap](../roadmap.md), [roadmap format](../documentation.md#roadmap), [roadmap skill](../../.agents/skills/roadmap/README.md), Lab design `2026-11-21-roadmap-backlog.md`

Planned work had no fixed place. Follow-ups ended in the *Open points* of design documents, and the Lab quality roadmap is a design document for one programme with phases. This change gives Kinetra a standing backlog in `docs/roadmap.md`, a format shared with Kinetra-Lab, and a `roadmap` skill that agents use while brainstorming, designing and finishing a feature.

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

- Follow-up work is scattered: open points on module READMEs, in design documents (for example the missing skill READMEs in [documentation cleanup](2026-11-19-docs-cleanup.md#open-points)) and in the Lab quality roadmap.
- Ideas from a brainstorm with the user, such as Android support or 3D rendering, have no place to land, so they get lost between sessions.
- The `feature` skill ends a branch without asking what work it leaves behind.

## Goals and non-goals

- Goal: one backlog page per repository with a fixed format, and rules for when items move or leave.
- Goal: skills that keep it current: `feature` reads and updates it, `grilling` sends deferred branches to it, and a new `roadmap` skill handles brainstorms and grooming.
- Non-goal: replacing design documents or ADRs. Non-goal: closing the Lab quality roadmap; its phase 8 and Demo's contact tests (phase 5, step 4) are still open, and are items here.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| One roadmap for both repositories, or one each | One each (user's choice) | Engine work and application work stay apart; an item that needs both appears in each, linked |
| What happens to the quality roadmap | Left untouched; its engine parts are listed in the new roadmap (user's choice) | Other sessions were updating it for phases 5 to 7 when this started; editing it would cause conflicts |
| Where the format is defined | The documentation guide, one section | Both repositories follow it; one authoritative place |
| Done items | Removed, no done list | The design document is the record; a done list grows without being read |
| Skill or only rules | A new `roadmap` skill, plus steps in `feature` and `grilling` | A brainstorm is not a feature; it needs its own trigger and steps |

## Design

Four sections (Now, Next, Later, Ideas), one list item per piece of work, moves only on the user's agreement. `feature` checks the roadmap in *Understand*, moves the item to Now when the branch starts, and removes it in *Document*, adding the follow-ups it leaves.

## Affected areas

`docs/roadmap.md` (new), `docs/documentation.md`, `docs/workflow.md`, `docs/README.md`, `AGENTS.md`, `.agents/skills/README.md`, `.agents/skills/roadmap/` (new), `.agents/skills/feature/SKILL.md`, `.agents/skills/grilling/SKILL.md`, this document and the design index.

## Test plan

Documentation only: the Docs level (`python3 tools/check_docs.py`).

## Review

Read through the diff for duplicated rules: the format lives only in the documentation guide; the skill and the workflow link it.

## Verification

- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**docs(roadmap): add a backlog and a roadmap skill**

- **Summary:** a standing backlog in `docs/roadmap.md`, its format in the documentation guide, and a `roadmap` skill. `feature` and `grilling` now keep it current.
- **Changes:** roadmap page with the open engine work and the user's ideas (Android runtime, 3D rendering); documentation guide section *Roadmap*; skill `roadmap` with README; steps in `feature`, `grilling` and the workflow; AGENTS.md rules and indexes.
- **Design:** this document.
- **Verification:** Docs level, green.
- **Dependencies and merge order:** merge before the Kinetra-Lab pull request that adds Lab's roadmap and moves the submodule pointer; use a merge commit.
- **Follow-up:** skill READMEs for the other seven skills (roadmap item).

## Open points

- The other seven skills still lack a README; listed under Next in the roadmap.
