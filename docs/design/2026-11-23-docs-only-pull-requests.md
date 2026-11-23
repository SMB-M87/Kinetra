# Docs-only pull requests without agent review

Status: implemented · Branch: `ccr-6e6399fb-049f2v` · Related: [Kinetra-Lab design](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-23-leaner-cycle-first-step.md) (Lab's CI skip and the measurement), roadmap item *Leaner agentic cycle*, [Git rules](../../AGENTS.md#git)

On 2026-11-23 the user asked that a pull request with only documentation, or only a submodule move, get no agent review. Pointer-only pull requests already had none; this change exempts docs-only pull requests too, in both repositories, as the first step of *Leaner agentic cycle*. Lab's CI, which now skips their build, changes in the Lab pull request of the same branch.

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

- Every pull request except a pointer-only one waits for a separate review session ([rule](../../AGENTS.md#git)). From 18 to 23 November, 15 of Kinetra's 36 merged pull requests changed only Markdown, `docs/` or `.agents/`, and so did 9 of the 57 that Lab merged into `main`, 6 more with a pointer move beside them.
- A review session costs a session start, a code review at depth `low` and a discussion round, for a diff that cannot change what is built. The findings on such pull requests were missed copies of a reworded rule, which the author's own step 6 and `check_copies.py` now look for.

## Goals and non-goals

- Goal: a docs-only pull request needs no agent review; the author merges it once CI is green, unless the merge rule leaves it to the user.
- Goal: every page that states the review rule says the same.
- Non-goal: a CI skip in Kinetra. Its full run took a median 1.8 minutes over 95 runs from 18 to 23 November; Lab's took 8.9.
- Non-goal: a smaller design document for docs-only changes, and the other candidate measures of the roadmap item. The user settles those after the measurement in Lab's design document.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Which pull requests | Only Markdown, `docs/` or `.agents/` changed; in Lab also a moved pointer whose Kinetra range holds only those paths, `.github/` or `tools/` | The same paths for which Lab's CI skips the build, so the exemption can be checked from the file list. Comment-only code changes stay reviewed: a file list cannot tell them apart |
| Merge | The author merges once CI is green, with no conflict and the stack below merged | As for a pointer-only pull request. The merge rule's list still leaves the agent rules, CI, releases and the user's checks to the user, also when they are only Markdown |
| The author's own review | Stays: step 6 of the workflow, the copies check included | Cheap, and the place where the missed copies of #26, #27, #30 and #35 are found |
| A review on request | The user can still start `pr-review` on a docs-only pull request | The exemption removes a waiting step, not the option |

## Design

- `AGENTS.md`: the review rule names docs-only pull requests next to pointer-only ones; a new **Docs-only pull requests** rule defines them; the merge rule says they need no review conclusion; the *Why* says why they need no second agent; the `pr-review` row of the skills table.
- `docs/workflow.md`: the cycle diagram sends docs-only and pointer-only pull requests from green CI to the merge; step 6 and step 8 name the exemption; the Kinetra range that Lab's CI skips now also lists `.github/` and `tools/`, and Lab's docs-only skip.
- Skills: `pr-review` (description, introduction, README), `pr-feedback` (agent reviews, merging), `feature` (steps 5 and 7), `verify` (the pointer range), and the skills overview.

## Affected areas

The pages above, Kinetra's roadmap item *Leaner agentic cycle* (to *Now*), this document and the design index. No code, CMake or CI.

## Test plan

Docs level: `python3 tools/check_docs.py`, and `python3 tools/check_copies.py` with the old wordings of the review rule and of the pointer range.

## Review

- The author's review of the diff, and the copies check with `--term 'except a pointer-only'`, `--term 'every pull request except'`, `--term 'Every pull request gets'`, `--term 'needs no review'`, `--term 'Markdown, docs or skills'` and `--term 'Markdown, \`docs/\` or \`.agents/\`'`: every hit in Kinetra and Lab either follows in these two pull requests or is a dated design document or review lesson, which stay as records.
- No agent review: this pull request is docs-only, and the user asked for none on such pull requests. It changes agent rules, so the user merges it.

## Verification

- `python3 tools/check_docs.py`: all links resolve.
- `python3 tools/check_copies.py` with the terms above: only the hits listed under *Review*.

## Pull request

**docs(agents): docs-only pull requests need no agent review**

- **Summary:** a pull request that changes only Markdown, `docs/` or `.agents/` no longer waits for a review session; the author merges it once CI is green, unless the merge rule leaves it to the user. The first step of *Leaner agentic cycle*, at the user's request of 2026-11-23.
- **Changes:** `AGENTS.md` (review rule, docs-only rule, merge rule, *Why*, skills table); `docs/workflow.md` (diagram, steps 6 and 8, Lab's skip); skills `pr-review`, `pr-feedback`, `feature`, `verify` and the overview; roadmap; this document.
- **Design:** this document; the Kinetra-Lab design of the same branch for the CI skip and the measurement.
- **Verification:** Docs level, `check_docs.py` and `check_copies.py`.
- **Review notes:** no agent review, by the user's request; the agent rules change, so the user merges.
- **Dependencies and merge order:** this pull request first, with a merge commit; then the Kinetra-Lab pull request of the same branch; then a pointer-only pull request, which is the first one judged by Lab's new skip.
- **Follow-up:** the further measures of *Leaner agentic cycle*, once the user has chosen them.

## Open points

- Whether docs-only changes still need a design document of their own is one of the measures the user settles next.
