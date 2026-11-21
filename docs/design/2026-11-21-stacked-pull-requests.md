# Stacked pull requests

Status: implemented · Branch: `docs/stacked-pull-requests` · Related: [workflow](../workflow.md#stacked-pull-requests), [agent rules](../../AGENTS.md#git), design `2026-11-21-roadmap-priorities.md` (on `docs/roadmap-priorities`, stacked on this branch), Lab design `2026-11-21-roadmap-priorities.md`

The user asked for a standing rule: work in stacked pull requests where possible, so a goal arrives as small increments that each can be reviewed and merged on their own. Several Lab phases already worked this way without a written rule. This change writes the rule into the agent rules, the workflow and the two skills that create and update pull requests.

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

- The rules say one branch per change and a pull request against `main` ([AGENTS.md](../../AGENTS.md#git), [workflow step 8](../workflow.md#8-commit-push-and-open-a-pull-request)). Nothing says how a goal with several dependent steps is split, which base a later step uses, or what happens to the steps above when a lower one gets review commits.
- Lab used stacks anyway, each time with its own conventions: phase 6 (Dotio) had seven stacked pull requests, phase 7 three, the Windows zip was stacked on the HTTPS submodule change (Lab design documents `2026-11-21-dotio-restructuring.md`, `2026-11-21-i4sim-passing-under-pressure.md`, `2026-11-21-windows-release-zip.md`). The merge order was written into each description by hand.
- `pr-feedback` pushes a fix to one branch and stops; in a stack, the branches above it then lack the fix and drift into conflicts.

## Goals and non-goals

- Goal: a rule in the agent rules that agents prefer stacked pull requests where a goal splits into reviewable steps.
- Goal: one place that says how: when to stack, branch and base, the description, verification, merging feedback up, and what happens after a merge.
- Goal: `feature` plans and opens stacks; `pr-feedback` merges a fix up the stack.
- Non-goal: a stack tool (such as git-spr or Graphite). Plain `git merge` and the base of a pull request are enough at this size.
- Non-goal: a skill that reviews pull requests. The user asked for one too; it is a roadmap item, because it needs its own design (how the author's session hears about the review, how agent comments are told apart from the user's).

## Decisions

All taken by the agent from the existing rules and practice; the user set the goal.

| Decision | Choice | Why |
| --- | --- | --- |
| Where the rule lives | Kinetra's AGENTS.md (the rule) and workflow (the how) | Lab applies all Kinetra rules; one place for both repositories |
| How changes travel up a stack | Merge the lower branch into the one above | The existing rule forbids rebasing published history and force-pushes; review comments stay on their commits |
| Merge strategy | Bottom-up, with a merge commit | A squash gives the steps above other commits than the ones they were built on; the Kinetra pointer rule already asks for merge commits |
| Design documents | One per stack, written on the first branch, with a table of steps and branches | That is how phases 5 to 7 worked; one place for the goal's decisions |
| Local Full level | On the top of the stack, and on a lower branch that changes behavior CI does not run | CI checks every pull request on its own (CI runs for every base); the top contains every step below it |
| Unrelated changes | Not stacked, unless they edit the same lines | A stack makes the upper change wait for the lower one; for shared lines (an index, a roadmap) that wait is cheaper than a conflict. Phase 7 stacked for this reason |

## Design

- [AGENTS.md, Git](../../AGENTS.md#git): the rule, with its reason, and a pull request's base is `main` or the branch below.
- [Workflow](../workflow.md#stacked-pull-requests): a new section with a diagram, when to stack, how, and keeping the stack current; step 2 (design) and step 8 (pull request) point to it; the branch row of *Branches and commits* allows a base below.
- `feature` decides on a stack in step 2, branches from the step below, and opens the pull requests bottom-up with the branch below as base. `pr-feedback` fetches the neighbors of a stacked branch and merges its fix up.

## Affected areas

`AGENTS.md`, `docs/workflow.md`, `.agents/skills/feature/SKILL.md`, `.agents/skills/pr-feedback/SKILL.md`, `.agents/skills/README.md` (the two rows), this document and the design index. Neither skill has a README yet ([skills overview, open points](../../.agents/skills/README.md#open-points)), so only the overview changes.

## Test plan

Documentation only: the Docs level (`python3 tools/check_docs.py`). This change is itself the first step of a stack of two; the `docs/roadmap-priorities` branch above it follows the new rule.

## Review

Read the diff against the rules it touches: the force-push and rebase ban, the Kinetra-before-Lab order and the pointer rule stay as they were; the new section links them instead of repeating them.

## Verification

- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**docs(workflow): work in stacked pull requests where possible**

- **Summary:** agents split a goal into small pull requests that are stacked on each other where possible. The rule is in AGENTS.md; the workflow says when to stack and how to keep a stack current; `feature` and `pr-feedback` follow it.
- **Changes:** AGENTS.md Git rule; workflow section *Stacked pull requests* and links from steps 2 and 8; `feature` step 2 and 9; `pr-feedback` steps 1 and 5; skills overview.
- **Design:** `docs/design/2026-11-21-stacked-pull-requests.md`.
- **Verification:** Docs level, green.
- **Dependencies and merge order:** bottom of a stack of two: merge this first, then `docs/roadmap-priorities`. The Kinetra-Lab pull request on `ccr-cc3e854a-v10cnx` records the top of this stack and merges after both, with merge commits.
- **Follow-up:** a skill that reviews pull requests (roadmap item *Pull request review skill*).

## Open points

- None for this change. A skill that reviews pull requests and hands its comments to the author's session is on the [roadmap](../roadmap.md).
