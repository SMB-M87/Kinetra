# Every copy of a changed rule

Status: implemented · Branch: `docs/copies-rule` (stacked on `docs/pointer-only-prs`) · Related: [review lessons](../review-lessons.md), [agent rules, repository documentation](../../AGENTS.md#repository-documentation)

The review lesson "when a change alters a procedure that more than one page states, find every copy and change them in the same commit" (from #26) came back in #27, where two copies were missed. The lessons page says a lesson that keeps coming back is proposed to the user as a rule. The user agreed on 2026-11-22. This change turns the lesson into a rule and makes the search a step of the author's own review and of the agent review.

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

- In #26, `feature` step 9.7 got a new procedure while the workflow paragraph it cites kept the old one.
- In #27, the `pr-review` README and the `verify` table kept the old statements, although the author had searched, with too narrow words.
- A lesson only informs; nothing in the review steps made the search happen.

## Goals and non-goals

- Goal: a rule in AGENTS.md, and the search as a step of `feature`'s review, of the workflow's review checklist and of `pr-review`'s checklist.
- Non-goal: a tool that finds copies automatically. A `grep` for the old wording is enough for pages of this size.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Promote the lesson | Yes: the rule moves to AGENTS.md (*Repository documentation*), and the lesson leaves the lessons page | The user's word, as the lessons page's format asks for a lesson that recurs |
| Where the search happens | The author's review (`feature` step 7, workflow step 6) and the agent review (`pr-review` checklist) | The author fixes it before the push; the reviewer catches what the author missed |
| How to search | The old wording and its key words, one command: `grep -rn --include='*.md' --include='*.yml'` from the repository root (in Kinetra-Lab that covers `external/Kinetra`) | #27 showed that one exact phrase is too narrow; the agent review of #29 asked what "docs and skills" covers, and a command leaves no doubt |
| When a missed copy is fixed | In the same commit, or in the fix commit when review finds it | Review findings are fixed in new commits (`pr-feedback`), so "the same commit" alone could never be met |

## Design

AGENTS.md gains the rule under *Repository documentation*. The workflow's review checklist gains **Copies**, and `feature` step 7 gains the search before the final commit. `pr-review` gains a checklist row; its description and README name it. The lesson leaves `docs/review-lessons.md`.

## Affected areas

`AGENTS.md`, `docs/workflow.md`, `docs/review-lessons.md`, `.agents/skills/feature/SKILL.md`, `.agents/skills/pr-review/SKILL.md` and `README.md`, this document and the design index.

## Test plan

Documentation and skills only: the Docs level.

## Review

- Self-review: applied the new rule to this change, searched for copies of the review checklist summary and found the `pr-review` description and README, both changed.
- Agent review on 99c46f9 (*Fix first*): the four copies of the new rule did not say the same (the workflow and `pr-review` copies dropped the key words and the Lab scope), the scope of "docs and skills" was unclear, "before the final commit" disagreed with "in the same commit", and the source was "#26 and #27" instead of "from #26, back in #27". All four copies now carry the same command and wording.

## Verification

- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**docs(agents): change every copy of a changed rule**

- **Summary:** the review lesson from #26, which came back in #27, becomes a rule. A change to a rule, procedure or term searches for every copy and changes them together. The author's review and the agent review both include the search.
- **Changes:** AGENTS.md rule; workflow review checklist; `feature` step 7; `pr-review` checklist, description and README; the lesson leaves the lessons page.
- **Design:** `docs/design/2026-11-22-copies-rule.md`.
- **Verification:** Docs level.
- **Dependencies and merge order:** stacked on #27; merge that first. It changes agent rules, so the user merges it.

## Open points

- None.
