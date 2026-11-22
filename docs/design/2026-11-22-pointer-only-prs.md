# Pointer-only pull requests

Status: implemented · Branch: `docs/pointer-only-prs` · Related: [review loop](2026-11-22-pr-review-loop.md), [changes across Kinetra and Lab](../workflow.md#changes-across-kinetra-and-lab), Kinetra-Lab's CI change of the same date

Moving Lab's submodule pointer to a Kinetra merge commit ran the whole review loop and CI each time, or re-ran the CI of a Lab pull request that was already reviewed. The user decided on 2026-11-22: a pull request that only moves the pointer needs no agent review, and the agent merges it itself. A pointer move always goes in such a pull request of its own, and Lab's CI skips build and tests for it when Kinetra changed no engine code. This change writes that into the rules; Lab's CI change is a Kinetra-Lab pull request.

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

- Kinetra-Lab #66 and #68 moved only the pointer, yet each waited for CI for about ten minutes, #68 also for an agent review and for the user's merge.
- The rule "move the pointer on the Lab branch before the Lab pull request merges" pushes a commit onto a reviewed pull request, which runs its CI again; when the user merged first (#65, #67), a pointer pull request followed anyway.

## Goals and non-goals

- Goal: a pointer-only pull request merges without review, by the agent.
- Goal: a pointer move is always its own pull request.
- Goal: no build and tests for a pointer-only pull request when Kinetra changed only Markdown, docs or skills.
- Non-goal: skipping CI when the pointer brings engine code: Kinetra's CI does not build Lab, so Lab's CI must.

## Decisions

Settled with the user on 2026-11-22.

| Decision | Choice | Why |
| --- | --- | --- |
| Review | None for a pointer-only pull request to a commit on Kinetra `main` | The user's word; the Kinetra commit was reviewed and merged in Kinetra |
| Merge | By the agent, once CI passes | The user's word; it is a mechanical follow-up the rules ask for right away |
| Separate pull request | Always; never a pointer commit on an open or reviewed pull request | The user's word: a reviewed pull request should not run CI again for one line |
| CI | Skip build and tests when Kinetra changed only Markdown, `docs/` or `.agents/` between the two pointers (an identical tree included), run only the docs check then; otherwise full CI | The user's choice over always skipping: a pointer can bring engine code that only Lab's CI tests against Lab |
| How CI skips | Jobs that start and are skipped by a condition, not a path filter on the workflow | `main` requires the `CI / checks` status; a filtered-out workflow never reports it, a skipped job reports success |

## Design

- [AGENTS.md, Git](../../AGENTS.md#git): a *Pointer-only pull requests* rule; the merge rule's order ends with the pointer pull request; the agent review's exception.
- [Workflow](../workflow.md#changes-across-kinetra-and-lab): the Lab pull request merges at the verified Kinetra branch commit, then a pointer-only pull request follows; the sequence diagram shows it.
- `feature`, `pr-feedback`, `pr-review`, `verify` and the skills overview name the exception and the order.

## Affected areas

`AGENTS.md`, `docs/workflow.md`, `.agents/skills/feature/SKILL.md`, `.agents/skills/pr-feedback/SKILL.md`, `.agents/skills/pr-review/SKILL.md`, `.agents/skills/verify/SKILL.md`, `.agents/skills/README.md`, this document and the design index.

## Test plan

Documentation and skills only: the Docs level. The CI behavior is tested on Kinetra-Lab's pull request and its first pointer-only pull request.

## Review

Searched every statement that each pull request gets an agent review, and every mention of the pointer order, and changed them together (the lesson of #26).

## Verification

- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**docs(workflow): pointer-only pull requests without review, merged by the agent**

- **Summary:** a Kinetra-Lab pull request that only moves the pointer to a commit on Kinetra `main` needs no agent review and is merged by the agent once CI passes; a pointer move is always its own pull request.
- **Changes:** AGENTS.md (rule, merge order, review exception); workflow (order, diagram); `feature`, `pr-feedback`, `pr-review`, `verify`, skills overview.
- **Design:** `docs/design/2026-11-22-pointer-only-prs.md`.
- **Verification:** Docs level.
- **Dependencies and merge order:** independent of Kinetra-Lab's CI change; both change rules or CI, so the user merges them. Lab records this in a pointer-only pull request afterwards.

## Open points

- None.
