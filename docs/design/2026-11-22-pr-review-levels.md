# Review depth by the kind of change

Status: implemented · Branch: `feat/pr-review-levels` · Related: [review loop](2026-11-22-pr-review-loop.md), [pr-review levels](../../.agents/skills/pr-review/SKILL.md#levels)

The first agent review ran its built-in code review at `medium`, the default the skill had, and its session at the platform's default effort. The user asked for a deeper look: the reviewer's session always on the most capable model at effort `high`, and at `xhigh` for a large pull request with much logic. The code review's level should follow the kind of change. This change adds a *Levels* table to `pr-review` and makes `feature` and the workflow start the review with it.

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

- `pr-review` ran `/code-review` at `medium` for every pull request, whether it held only Markdown or a change of simulation behavior.
- Nothing set the reviewer session's own effort; it ran at the default, although a reviewer needs the more critical look.

## Goals and non-goals

- Goal: the reviewer's session runs on the most capable model the agent offers, at high reasoning effort, and one level higher for a large pull request with much logic.
- Goal: the code-review level follows the kind of change.
- Goal: agent-neutral wording, so Codex and other agents can follow the rules as well as Claude Code (the user's request): no model names, depth instead of product settings, and agent-specific commands only as examples.
- Non-goal: naming a model in the repository; the agent that starts a review picks the most capable model it offers.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Reviewer session | The most capable model; effort `high`, `xhigh` for a large pull request with much logic | The user's choice: a reviewer needs a critical look, more for much logic |
| "Large, with much logic" | More than about 300 changed lines of `.h`/`.cpp` outside `tests/`, a change of simulation behavior (golden, characterization or pressure outcomes), or a change across Kinetra and Lab at once | Proposed by the agent: measurable from the diff, and the cases where a missed bug costs most |
| Code-review level | `low` for Markdown, skills and comments; `medium` for code in one module, tests and scripts; `high` for public headers, simulation behavior, timing or threads, CMake, CI, the release, or a Lab pointer to engine sources | The user's request, with the agent's split; `max` stays reserved for an explicit request, as the built-in review asks |
| Who sets them | The author, when it starts the review. The reviewer raises the code-review depth itself when the diff is riskier; an effort below the table becomes a point in its summary, because a session cannot raise its own effort or model | The author knows the diff first; the reviewer is the check. Found by the agent review of #26 |
| Depth above `high` | Not a default; `xhigh` or `max` runs only when the author or the user passes it | The user chose three depths (low, medium, high); the first commit dropped `xhigh` from the hint by accident, the agent review of #26 asked |
| Several modules, runtime and launcher | `high`, as the verification table puts them at Full | Found by the agent review of #26: no row covered them |
| Agent-neutral wording | The levels describe depth (high reasoning effort, one level higher where the agent has one; code review `low`, `medium`, `high` with their meaning); starting the review says what to do (a new session or task, else a fresh sub-agent, else ask the user); the footer names the agent that wrote it; Claude Code's commands appear only as examples. AGENTS.md and the skills overview make this a rule for all skills | The user's request: the rules must work for Codex too, and must not name a model |

## Design

`pr-review` gains a *Levels* section with both tables, and step 4 uses it. `feature` step 9.7 starts the review with that depth, and the workflow's agent review paragraph names it. The skill's README explains the depth. A second commit makes the wording agent-neutral (the footer, the GitHub tools, following a pull request, starting the review) and adds the rule to AGENTS.md and the skills overview.

## Affected areas

`.agents/skills/pr-review/SKILL.md` and `README.md`, `.agents/skills/feature/SKILL.md`, `.agents/skills/README.md`, `docs/workflow.md`, `AGENTS.md` (Skills), this document and the design index.

## Test plan

Documentation and skills only: the Docs level (`python3 tools/check_docs.py`). Its own agent review runs at effort `high` with the code review at `low`, as the table gives for a skills-only diff.

## Review

- Self-review: the author read the branch diff against the user's two requests (levels, agent-neutral wording) and ran the docs check, but did not run a code review on it and did not search for other copies of the changed procedure; that is why the workflow paragraph kept the old way to start the review.
- First agent review (on 22c8e52, *Fix first*): workflow and `feature` disagreed, the reviewer could not raise its own effort, no depth for several modules or runtime behavior, a question on depths above `high`, and nits on this document, `pr-feedback` wording and the footer. All addressed in the next commit; lesson added to [review lessons](../review-lessons.md#docs-and-links).
- The thresholds are the agent's proposal; the user can adjust them on this pull request.

## Verification

- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**feat(skills): review depth by the kind of change**

- **Summary:** the reviewer's session runs on the most capable model at high reasoning effort (one level higher for a large pull request with much logic), and the code review at `low`, `medium` or `high` by the kind of change; skills and rules stay agent-neutral.
- **Changes:** *Levels* in `pr-review` with step 4 using it; `feature` step 9.7; the workflow's agent review; the skill's README; agent-neutral wording in `pr-review`, `pr-feedback`, `feature` and the workflow, and the rule for it in AGENTS.md (*Skills*) and the skills overview.
- **Design:** `docs/design/2026-11-22-pr-review-levels.md`.
- **Verification:** Docs level.
- **Dependencies and merge order:** none in Kinetra; it changes skills, so the user merges it. Kinetra-Lab records it in a pointer commit afterwards.

## Open points

- The thresholds will be tuned by use.
