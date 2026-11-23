# Copies check

Status: implemented · Branch: `feat/copies-check` · Related: [Every copy of a changed rule](2026-11-22-copies-rule.md), review of #35

A missed copy of a changed name came back for the fourth time (#26, #27, #30, #35) despite the rule in [AGENTS.md](../../AGENTS.md#repository-documentation). The user decided on 2026-11-23 to add a mechanical aid: `tools/check_copies.py` lists the names a change removes that a page still names, and the old wordings it is given, and the `verify` skill runs it at every level.

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

The rule asks for a `grep` on the old wording and each key word. It depends on the author choosing the right words: #30 searched for one copy's sentence, and #35 searched for "`ubuntu` job" but not "job `ubuntu`", so `docs/testing.md` kept naming a job that the pull request removed. Kinetra-Lab#78 missed two copies of "skipped on Windows" the same way. The reviewer proposed a step that finds removed identifiers mechanically.

## Goals and non-goals

Goals: a read-only script that lists, for the current diff, the removed CI jobs, CMake options, presets, flags and inline Markdown names that a page still names, plus every place an old wording given on the command line is left; the `verify` skill runs it at every level; the four places that state the `grep` instruction point to it.

Non-goals: finding a rule restated in other words (review still reads for that); failing CI on a hit (a hit is a candidate, and some names stay right where they are).

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| A mechanical aid | A script run by `verify` (the user, 2026-11-23) | The reviewer's proposal; a missed copy came back four times |
| Advisory or failing | Advisory by default, `--strict` for exit status 1 | A removed name can still be right elsewhere, such as a status name a roadmap item stops mentioning |
| What counts as a name | Job ids, `option(...)`, preset names, `-D` flags, inline code spans; plain lowercase words such as `main` skipped | These are what the missed copies were; plain words name too many things |
| Where it searches | `*.md`, `*.yml`, `*.yaml`, tracked and untracked, submodules included; dated design documents and review lessons skipped unless `--all` | The rule's scope; records of their time are not copies |

## Design

`tools/check_copies.py` takes the merge base of `--base` (default `origin/main`, else `main`) and HEAD, lists the files `git diff --name-only --no-renames <base>` reports (the working tree included), and compares each file's text at the merge base with its text now. Removed names get search patterns by kind: a job is matched as "`x` job", "job `x`", "job id "x"", "CI / x" or in `needs:`; a preset as `--preset x`, "`x` preset" or "preset `x`"; the others by name. `--term` adds case-insensitive wordings. The output is one block per name with up to eight `path:line: text` hits.

## Affected areas

`tools/check_copies.py` and [tools](../../tools/README.md#check_copiespy); the copies rule in [AGENTS.md](../../AGENTS.md#repository-documentation), the review step of the [workflow](../workflow.md#6-review), the `feature`, `verify` and `pr-review` skills and the `verify` README; this document and the [index](README.md#index).

## Test plan

| Behavior | Proof | Level |
| --- | --- | --- |
| It finds #35's missed copy | In a worktree of #35's first commit, `--base d67d83f` lists `docs/testing.md:126` under CI job `ubuntu` | Docs (a script that only reads; no build) |
| `--term` finds Kinetra-Lab#78's missed copy | In a Lab worktree of `9b86757`, `--base 37ee53d --term 'skipped on Windows'` lists `labs/dotio/README.md:373` | Same |
| Exit codes | `--strict` with a hit gives 1; an unknown `--base` gives 2 | Same |
| No noise on its own branch | No hit for the removed `grep` instruction | Same |

## Review

Filled in after review.

## Verification

The four rows of the test plan, run by hand on 2026-11-23: #35's copy found (and one more hit, `CI / checks` in the workflow page, which is right where it is); Lab's copy found with `--term`; exit 1 with `--strict`, 2 with an unknown ref; on this branch no hit for the old `grep` wording. Before the plain-word filter, Lab's run also listed 30 hits for `main` and `windows`. `python3 tools/check_docs.py` passes.

## Pull request

Title: `feat(tools): list the copies of names a change removes`

**Summary.** A missed copy of a changed name came back for the fourth time (#26, #27, #30, #35). `tools/check_copies.py` lists the CI jobs, CMake options, presets, flags and inline names a diff removes that a page still names, plus any old wording given with `--term`, and the `verify` skill runs it at every level (the user's decision of 2026-11-23).

**Changes.**
- `tools/check_copies.py`, read-only, advisory unless `--strict`; documented in `tools/README.md`.
- The `verify` skill runs it at every level and reports it; its README has a use case.
- The copies rule in AGENTS.md, the workflow's review step and the `feature` and `pr-review` skills name the script instead of a hand-written `grep`.

**Design.** `docs/design/2026-11-23-copies-check.md`.

**Verification.** It finds the copy #35's review found and, with `--term`, the one Kinetra-Lab#78's review found; exit codes checked; docs check passes. A script that only reads: the Docs level, plus this pull request's CI.

**Review notes.** Hits are candidates, not errors. Kinetra-Lab gets the script through the pointer.

**Dependencies and merge order.** Independent; changes agent rules and skills, so it waits for the user.

**Follow-up.** Kinetra-Lab's `AGENTS.md` can name the script in its verification table once the pointer records it.

## Open points

- It does not find a rule restated in other words; review still reads for those.
