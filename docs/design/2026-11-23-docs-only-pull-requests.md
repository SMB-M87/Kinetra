# Docs-only pull requests, one design document, and less to read

Status: implemented · Branch: `ccr-6e6399fb-049f2v` · Related: Kinetra-Lab's design document `docs/design/2026-11-23-leaner-cycle-first-step.md` in [Kinetra-Lab#83](https://github.com/SMB-M87/Kinetra-Lab/pull/83) (the measurement and Lab's CI skip), roadmap item *Leaner agentic cycle*, [Git rules](../../AGENTS.md#git), [development cycle](../../AGENTS.md#development-cycle)

On 2026-11-23 the user asked that a pull request with only documentation, or only a submodule move, get no agent review, and then chose four measures from the measurement in Lab's design document. Three of them change Kinetra: a docs-only change needs no design document, a change across both repositories gets one design document, and Kinetra's CI skips the build of a docs-only pull request. The user also asked whether agents must read the design documents at all; they no longer do at the start of a task. The first step of *Leaner agentic cycle*.

This change spans both repositories and has a design document in each, against the rule it adds: both were written before the user chose that rule, halfway through the branch. Kinetra's holds the rules and Kinetra's CI, Lab's the measurement and Lab's CI.

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

- Every pull request except a pointer-only one waited for a separate review session. From 18 to 23 November, 15 of Kinetra's 36 merged pull requests changed only Markdown, `docs/` or `.agents/`, and so did 9 of the 57 that Lab merged into `main`, 6 more with a pointer beside them. Since the agent review started on 22 November, Kinetra #26 to #29 and Lab #67 each got one.
- Every docs-only pull request also got a design document, and 12 topics have one in both repositories.
- The design documents now hold more text than the code: about 70k tokens here and 170k in Lab, against roughly 110k and 300k tokens of C++. The routing table sent agents to them for current and planned work, so a task could start by reading history.
- Kinetra's CI built and tested every docs-only pull request, a median 1.8 minutes over 95 runs.

## Goals and non-goals

- Goal: a docs-only pull request needs no agent review and no design document, and Kinetra's CI runs only the docs check for it.
- Goal: a change across Kinetra and Lab has one design document.
- Goal: agents read a design document only when something links it, or a search for the topic finds it.
- Goal: every page that states these rules says the same.
- Non-goal: moving, shortening or deleting the existing design documents; the user keeps them.
- Non-goal: the automatic pointer pull request, which Lab's roadmap holds.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Which pull requests are docs-only | Only Markdown, `docs/` or `.agents/` changed; in Lab also a moved pointer whose Kinetra range holds only those paths, `.github/` or `tools/` | Close to the paths for which CI skips the build, so the exemption is checked from the file list; Lab's CI still builds the few Lab paths a test reads or the build copies ([Lab's rule](https://github.com/SMB-M87/Kinetra-Lab/blob/main/AGENTS.md#verification-in-lab)). Comment-only code changes stay reviewed: a file list cannot tell them apart |
| Merge | Under the merge rule: CI green, every review thread resolved, no conflict, every pull request below merged, in Kinetra-Lab including the Kinetra pull request its pointer records | As for any pull request, without the review conclusion. The merge rule still leaves the agent rules, CI, releases and the user's checks to the user, also when they are only Markdown |
| The author's own review | Stays, the copies check included | Cheap, and the place where the missed copies of #26, #27, #30 and #35 are found |
| Design document of a docs-only change | None; the pull request description holds the problem, the decisions and the verification | The user's choice of 2026-11-23; a docs-only pull request is mostly the record of itself |
| A change across both repositories | One design document, in the repository with most of the change; the other pull request links it | The user's choice of 2026-11-23 |
| Reading design documents | Not at the start of a task: only the branch's own, one that is linked, or one a search for the topic finds | The user asked whether agents must load them; they are history once implemented, and READMEs describe the current state |
| CI skip in Kinetra | A `changes` job as in Lab, inline, for pull requests only; a push to `main` always builds | The user's choice of 2026-11-23. Kinetra has no pointer, so the decision is a few lines; the post-merge run stays complete |
| History for the decision | A blobless checkout (`filter: blob:none`) | The decision needs commits and trees only |

## Design

- `AGENTS.md`: the review rule names docs-only pull requests; a **Docs-only pull requests** rule defines them; the merge rule says they need no review conclusion; the development cycle gives the two design-document exceptions and the reading rule, and its *Why* says why; the routing table sends roadmap readers to a design document only when an item links it; the `pr-review` row.
- `docs/workflow.md`: the cycle diagram sends docs-only and pointer-only pull requests from green CI to the merge; step 2 has the exceptions and the reading rule; steps 6 and 8 the review exemption and the description without a design document; *Changes across Kinetra and Lab* the single design document and Lab's wider skip.
- `docs/design/README.md`: the exceptions, and that the index is for a search by topic.
- Skills: `pr-review` (description, introduction, checklist, README), `pr-feedback` (steps 3 to 5), `feature` (steps 2.3, 2.5, 5, 7, 8 and 9, and its README), `roadmap` (starting and closing an item, and its README), `verify` (the pointer range), and the overview.
- The other copies of the design-document and review rules: `README.md`, `docs/README.md` (the routing row), `docs/documentation.md` (the roadmap record), `docs/roadmap.md`, and the consumer note in `tests/README.md`; review rounds 1 and 2 found them.
- `tools/check_copies.py`: reads `git show` output with replacement characters, so a changed binary file no longer stops it (it stopped on Lab's deleted tour video), and skips a file whose old text holds a NUL byte, whose decoded bytes only gave chance matches.
- `.github/workflows/ci.yml`: a `changes` job decides with `git diff --no-renames --name-only` against the merge base, skips only when `grep` selects no source path (status 1), so the decision fails closed, and runs the docs check for a skipped pull request; `linux` needs it and runs unless it skips; `checks` passes on a rightful skip, as in Lab. `docs/testing.md` describes it.

## Affected areas

The pages above, `.github/workflows/ci.yml`, `tools/check_copies.py`, Kinetra's roadmap (*Leaner agentic cycle* leaves it: this document and Lab's are its record, and its last measure is Lab's item *Automatic pointer pull request*), the review lessons, this document and the design index.

## Test plan

- The decision step, extracted from `ci.yml`, in a blobless clone of Kinetra: the docs-only #29, #34 and this branch's first commit skip; the code and CI pull requests #33, #35 and #36 build; a source moved into `docs/` with `git mv` builds; a line in `ci.yml` builds; an empty diff skips; a base that does not exist warns *No merge base* and leaves `skip_build` empty, a full run.
- The fail-closed form of the decision (after review round 2), on its own under `bash -e` with and without `pipefail`: 20,000 source paths build in 20 of 20 runs, 20,000 docs paths skip, a mixed diff builds, an empty diff skips.
- YAML parses; `python3 tools/check_docs.py`; `python3 tools/check_copies.py` with the old wordings.
- `check_copies.py` in Kinetra-Lab on a branch that deletes `docs/media/kinetra-lab-tour.mp4`: a `UnicodeDecodeError` before the fix, the list of hits after it.
- This pull request's own CI changes `ci.yml`, so it builds in full; the skip runs on the next docs-only pull request.

## Review

- The author's review of the diff, and the copies check with the old wordings of the review rule (`except a pointer-only`, `every pull request except`, `Every pull request gets`, `needs no review`), of the pointer range (`Markdown, docs or skills`) and of the design-document rule (`Each branch has a design document`, `Every branch has a design document`, `its own branch, design document and pull request`): every hit follows in this pull request or the Kinetra-Lab one, or is a dated design document or review lesson.
- With the CI change this pull request is no longer docs-only, so it gets an agent review; it changes agent rules and CI, so the user merges.
- Agent review, round 1 (on 5246745, *Fix first*):
  - Blocking, copies that still gave every change a design document and a review, or every pull request a build (README, cycle and diagram, workflow steps 6 to 8, `feature` steps 8 and 9, its README, the `roadmap` closing step, the `pr-review` checklist, the docs index, the consumer note): fixed in 7f6ec4d, after a search on the key words `design document`, `every change` and `every pull request`.
  - The docs-only merge sentence dropped resolved threads and Kinetra before Lab: fixed in 7f6ec4d, and carried to workflow step 8, `feature` step 9.5 and `pr-feedback` step 5.
  - Kinetra's copies of Lab's skip left out the Lab paths that always build: they now link Lab's rule (16cbd03).
  - Two design documents for one change: both stay, explained above; the link to Lab's document goes through Kinetra-Lab#83, since this pull request merges first.
  - Nits: the diagram skips the design document for a docs-only change (7f6ec4d); `check_copies.py` skips binary files, which gave six chance `-D` flags on the deleted video (a5dcd89); the branch name comes from the session environment, which allows only this branch, as the pull request description says.
  - Two lessons added to the [review lessons](../review-lessons.md); the rule candidate on searching key nouns is the user's.
- Agent review, round 2 (on b13129d, *Fix first*), all carried out in one commit:
  - Blocking, steps that still wrote into a design document or called it the record: the roadmap skill, its README and diagram, the documentation guide's roadmap section and the roadmap's own lead now name the pull request as a docs-only change's record; `feature` step 2.3 and workflow step 2 send whatever later steps write into the design document to the description; the `pr-review` row, `pr-feedback` steps 3 and 4, the skills overview and the `feature` README example follow. Found this time by searching the document's parts (`Decisions`, `Test plan`, `Verification`, *Open points*, "is the record") as well as its name.
  - The *Docs-only pull requests* bullet merges "under the merge rule above" instead of repeating a short list; Lab's copy already links the rule.
  - The `changes` decision fails closed: tested with 20,000 source paths, a large docs diff, a mixed and an empty diff, with and without `pipefail`.
  - *Leaner agentic cycle* leaves Kinetra's *Now*; *Decisions*, *Design*, *Affected areas* and *Pull request* include the round-1 commits.
  - Lessons: the round-1 lesson on exemptions is replaced by the reviewer's wider one, and one on failing closed is added.
- Agent review, round 3 (on 2008388, *Fix first*, one point to fix and nits), all carried out:
  - Skills used on their own still wrote into a design document: `grilling` (description, step 4, README), `adr` (steps and README), the ADR bullet of workflow step 2, the `roadmap` README's note on bugs and the `pr-review` goal list now name the description for a docs-only change; the diagram keeps grilling for it, with the description in the design node; `docs/development.md` says CI runs the consumer on every pull request that changes more than documentation.
  - `feature` step 9.5 no longer merges; step 9.8 is the only merge step.
  - *Test plan* and *Verification* list the fail-closed runs; the double blank line in the review lessons is gone; the pull request description matches *Pull request* again.
  - 2008388 changed the review lessons inside a `docs(design)` commit; pushed history stays as it is, and the lesson edits of this round have a commit of their own.

## Verification

- The decision step's runs in the test plan, with the results above, the fail-closed runs included.
- `python3 -c "import yaml; …"` parses the workflow.
- `python3 tools/check_docs.py`: all links resolve.
- `python3 tools/check_copies.py` with the terms above: only the hits listed under *Review*.

## Pull request

**Leaner cycle: docs-only pull requests skip the review, the design document and the build**

- **Summary:** a pull request that changes only Markdown, `docs/` or `.agents/` gets no agent review and no design document, and Kinetra's CI runs only the docs check for it (Lab's CI skips most of them); a change across both repositories gets one design document; agents read design documents only when something links them. The first step of *Leaner agentic cycle*, with the user's choices of 2026-11-23.
- **Changes:** `AGENTS.md` (review, docs-only and merge rules, development cycle, routing table, skills table); `docs/workflow.md`, `docs/design/README.md`, `docs/README.md`, `docs/documentation.md`, `docs/roadmap.md`, `tests/README.md`, `README.md`; skills `pr-review`, `pr-feedback`, `feature`, `roadmap`, `verify` and the overview; `.github/workflows/ci.yml` and `docs/testing.md` (the `changes` job, failing closed); `tools/check_copies.py` (binary files); three review lessons; `grilling`, `adr`, `docs/development.md`; this document.
- **Design:** this document; Kinetra-Lab's design document in Kinetra-Lab#83 for the measurement and Lab's skip.
- **Verification:** the decision step in a blobless clone (docs-only skips; code, CI and a rename into `docs/` build; doubt builds) and the fail-closed form under both shells; YAML; `check_docs.py`; `check_copies.py` with the old wordings and the document's parts.
- **Review notes:** agent review rounds 1 and 2, both *Fix first*, answered in the commits named under *Review*. The branch name comes from the session environment.
- **Dependencies and merge order:** this pull request first, with a merge commit; then Kinetra-Lab#83 with its pointer at this branch's head; then a pointer-only pull request. The user merges both.
- **Follow-up:** Lab's *Automatic pointer pull request*; the reviewer's rule candidate on searching key nouns waits for the user.

## Open points

- The design index in `docs/design/README.md` sits below the template, so writing a new design document still loads it. If that weighs, the index can move to a page of its own.
