# Roadmap decisions of 2026-11-22

Status: implemented · Branch: `docs/roadmap-decisions` · Related: [roadmap](../roadmap.md), [roadmap priorities](2026-11-21-roadmap-priorities.md), Kinetra-Lab design `2026-11-21-roadmap-priorities.md` (its Lab half)

The roadmap order of 2026-11-21 was the agent's proposal. On 2026-11-22 the user settled it in a grilling session, together with the design of the pull request review skill. This document records the decisions that concern Kinetra; the Lab decisions (i4sim, Demo, Dotio, WSLg) are in Kinetra-Lab's design document of 2026-11-21.

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

- The roadmap's order and several items (*Pull request review skill*, *Linux beyond Arch*) still carried open questions for the user.
- The archive outage of 2026-11-21 suggested an item for a CI image, which had no place yet.

## Goals and non-goals

- Goal: the decisions recorded, and the roadmap items rewritten with them.
- Non-goal: building the review skill; it follows on `feat/pr-review-skill`, stacked on this branch.

## Decisions

Settled with the user on 2026-11-22. ⚑ marks answers that differ from the agent's recommendation.

| Decision | Choice | Why |
| --- | --- | --- |
| What comes first | Process and tooling: the review skill, ready-to-build cloud sessions, golden tests on every toolchain, graphics tests in CI; then the inspector overlay | Every later pull request becomes cheaper to verify |
| When an agent reviews a pull request ⚑ | Only on the user's request (`/pr-review`) | The user's choice; the agent recommended a review in a separate session after every pull request |
| What the review checks | The built-in `/code-review` for correctness, then a checklist from the rules | `/code-review` does not know Kinetra's rules; the skill must not duplicate it |
| What the review posts | Inline comments per finding, plus one summary review per pull request with a conclusion and points of attention | The author's session picks up the inline comments; the user reads the summary |
| Scope of one call | One pull request, or the top of a stack for all pull requests in it, bottom-up | Stacks are the normal way of working now |
| What the author's session does | It follows its pull requests and processes review comments with `pr-feedback` at once: carries out clear findings, answers disagreement with its reason, takes trade-offs to the user | Same rules as for the user's own feedback; the pull request does not wait |
| Learning from reviews (added by the user) | A lessons page per repository, `docs/review-lessons.md`, linked from AGENTS.md; only findings that show a kind of mistake that can recur; the author adds the lesson in a commit on the reviewed pull request; `feature` reads the page before building, `pr-review` before reviewing; a lesson that keeps coming back is proposed as a rule, and the user decides | The user wants agents not to repeat each other's mistakes; a page keeps lessons out of the rules until the user agrees |
| Linux beyond Arch | One extra CI job, on Ubuntu LTS; Fedora documented, not tested | Covers the Debian family and the cloud sessions at the cost of one job |
| CI image | A *Later* item | It removes the dependency on the archive, but can only be built while the archive answers |

Cosmetic choices accepted without comment: the skill is called `pr-review`; comments on GitHub in English, the hand-over in Dutch; `/code-review` runs at `medium` unless the call says otherwise; a second review round also starts only on request.

## Design

[Roadmap](../roadmap.md): the introduction says the order is settled; *Pull request review skill* holds the decided design; *Linux beyond Arch* names Ubuntu LTS; a new *Later* item *CI image with the toolchain built in*. The design document of 2026-11-21 points here.

## Affected areas

`docs/roadmap.md`, `docs/design/2026-11-21-roadmap-priorities.md` (open points), this document and the design index.

## Test plan

Documentation only: the Docs level (`python3 tools/check_docs.py`).

## Review

Each decision checked against the user's answers in the session; the ⚑ mark is on the one answer that differs.

## Verification

- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**docs(roadmap): record the decisions of 2026-11-22**

- **Summary:** the user settled the roadmap order and the design of the pull request review skill; the roadmap and this document record them.
- **Changes:** roadmap items *Pull request review skill* and *Linux beyond Arch*, new item *CI image with the toolchain built in*, settled order; this document.
- **Design:** `docs/design/2026-11-22-roadmap-decisions.md`.
- **Verification:** Docs level.
- **Dependencies and merge order:** bottom of a stack: this, then `feat/pr-review-skill`, each with a merge commit; Kinetra-Lab's pull request on `ccr-cc3e854a-v10cnx` records the top and merges last.
- **Follow-up:** the review skill on the next branch.

## Open points

- None.
