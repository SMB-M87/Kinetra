# Automatic Lab pointer pull request

Status: implemented · Branch: `ccr-ad716caf-1sr52q` in both repositories (set by the session environment) · Related: Kinetra-Lab's roadmap item *Automatic pointer pull request*, [Lab's design of the leaner cycle](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-23-leaner-cycle-first-step.md#open-points)

After every Kinetra merge, an agent session opened Kinetra-Lab's pointer-only pull request by hand, waited for its CI and merged it. A workflow in Kinetra now does that step: it opens the pull request, or adds a commit to the open one, and turns on auto-merge. It waits while a Lab pull request records its own pointer. The user made the settings it needs on 2026-11-23 (a fine-grained token stored as `LAB_POINTER_TOKEN` in Kinetra, auto-merge allowed in Lab) and chose the trigger and the waiting rule.

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

- Lab's `main` records one Kinetra commit. After each Kinetra merge, the rules call for a separate pointer-only pull request in Lab, which the agent merges once its CI passes ([rule](../../AGENTS.md#git)). 6 of the 57 Lab pull requests #24 to #81 were nothing else (Lab's measurement in its leaner-cycle design), and this session's Kinetra-Lab#84 and #88 were two more.
- Each one costs an agent step: a branch, a commit, a pull request, waiting for CI, a merge. When the session ends first, Lab's `main` lags behind Kinetra's until someone notices.

## Goals and non-goals

- Goal: after a Kinetra merge, Lab's pointer moves to Kinetra `main` without an agent session, through Lab's CI and its required `checks`.
- Goal: it never moves the pointer past a Lab pull request that records its own Kinetra commit; that pull request merges first, as the rules say.
- Goal: a failure shows as a red run, never as "nothing to do".
- Non-goal: Lab's CI and its decision on pointer-only pull requests stay as they are.
- Non-goal: a Lab pull request that follows a Kinetra branch during review still moves its pointer by hand, as the stack rules say.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Trigger | Kinetra's push to `main`, hourly, and by hand; chosen by the user on 2026-11-23 | Immediate after a merge; the hourly run catches up after a run that waited or failed. A scheduled Lab workflow would need the token in Lab too and is never immediate |
| An open Lab pull request that changes `external/Kinetra` | Wait, write nothing; chosen by the user on 2026-11-23 | That pull request merges with the pointer it was verified with; moving `main`'s pointer first would give it a conflict. The hourly run opens the pointer pull request after it merges |
| Credentials | A fine-grained token for Kinetra-Lab only (*Contents* and *Pull requests* read and write), stored in Kinetra as `LAB_POINTER_TOKEN`; the workflow's own token reads Kinetra | The workflow's own token cannot write to another repository, and a pull request it opened would not start Lab's CI. The user made the settings |
| How the commit is made | Through GitHub's Git data API: a tree with the new gitlink, a commit, a ref | No clone of Lab or Kinetra, and no Git credentials on the runner |
| One branch, `chore/kinetra-pointer` | An open pull request of the script gets one more commit, a fast-forward; a branch left by a closed one is deleted and made again from `main` | One open pointer pull request at a time; no force push |
| Merge | `gh pr merge --auto --merge` | Lab allows only merge commits on `main`; auto-merge waits for the required `checks` |
| Where the logic lives | `tools/lab_pointer.sh`, called by `.github/workflows/lab-pointer.yml` | A script runs locally with `--dry-run` and passes `shellcheck`; Kinetra's `tools/` is outside what Lab's build reads |
| Test before the first live run | The workflow runs as a dry run on a pull request that changes it | The write path can only run after the merge; the dry run proves the token, the reads and the decision on the pull request itself |

## Design

`tools/README.md` has the [flow and its diagram](../../tools/README.md#lab_pointersh). In short:

1. Read Kinetra `main` and Lab `main`'s gitlink; equal means done.
2. Check with Kinetra's compare API that `main` is ahead of the pointer; otherwise fail, since a person has to look.
3. List Lab's open pull requests; if one other than the script's own changes `external/Kinetra`, write nothing and say why.
4. Make a commit that changes only the gitlink, on top of the open pull request's branch or of Lab `main`, and move or create `chore/kinetra-pointer`.
5. Open the pull request, or update its title and description, with the Kinetra pull requests the move brings, and turn on auto-merge.

The workflow runs one writing run at a time (`concurrency`), and stops with an error when the secret is missing; a dry run on a fork's pull request, which gets no secrets, only warns.

## Affected areas

- Kinetra: `.github/workflows/lab-pointer.yml`, `tools/lab_pointer.sh`, `tools/README.md`, `docs/testing.md` (continuous integration), `AGENTS.md` (merge rule, pointer-only pull requests, functionality index), `docs/workflow.md` (diagram and *Changes across Kinetra and Lab*), `feature` step 9.4 and its README, `pr-feedback` step 5.5, this document and the design index.
- Kinetra-Lab: `AGENTS.md` (*Git with the engine submodule*), `external/README.md`, the roadmap item, removed.

## Test plan

- `bash tools/lab_pointer.sh --dry-run` against the real repositories: with Kinetra-Lab#89 open, which records its own pointer, it waits; with the wait turned off in a copy, it plans a pull request from 159a58a to Kinetra main and lists Kinetra#40.
- An unknown Lab repository stops it with status 1; an unknown argument with status 2.
- `shellcheck` on the script; the workflow parses as YAML.
- On this pull request: the workflow's dry run in CI, with the real secret, and Kinetra's full CI.
- After the merge: the first live run, watched by the author; its pull request in Lab merges through auto-merge.

## Review

- The author's review: the wait check reads each pull request's file list into a variable before `grep`, so `grep -q` cannot end `gh` early and turn a SIGPIPE under `pipefail` into "no pointer change" (Kinetra's lesson from #35 and #37). The copies search for `pointer-only`, `pointer pull request` and `merge commit` in both repositories found the places listed under *Affected areas*.

## Verification

- `bash tools/lab_pointer.sh --dry-run`: "Waiting: SMB-M87/Kinetra-Lab#89 changes external/Kinetra and merges first; a later run moves the pointer to 704237b."
- The copy without the wait: "Dry run: would open a pull request from chore/kinetra-pointer, moving the pointer from 159a58a to Kinetra main 704237b, and turn on auto-merge." and "- Kinetra#40: docs(rules): read sections, and search the roadmap and design index".
- `LAB_REPO=SMB-M87/does-not-exist`: status 1; `--bogus`: status 2.
- `shellcheck` 0.11.0: no findings. `python3 -c "import yaml; …"`: the workflow parses.
- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**ci: open Kinetra-Lab's pointer pull request automatically**

- **Summary:** after each Kinetra merge, a workflow opens Lab's pointer-only pull request and turns on auto-merge, so no agent session spends a step on it; it waits while a Lab pull request records its own pointer, and catches up hourly. The user's settings and choices of 2026-11-23.
- **Changes:** the workflow and its script; `tools/README.md` and `docs/testing.md`; the rules that named the agent's manual step (`AGENTS.md`, the workflow page, `feature`, `pr-feedback`); this document.
- **Design:** this document.
- **Verification:** the dry runs, the failure statuses, `shellcheck`, YAML, the docs check; this pull request's dry run in CI and Kinetra's full CI.
- **Review notes:** the wait check, the fast-forward update of an open pull request, and the token split between the repositories.
- **Dependencies and merge order:** this pull request first; then Kinetra-Lab's companion pull request, after Kinetra-Lab#89. It changes CI and agent rules, so the user merges it.
- **Follow-up:** watch the first live run.

## Open points

- The first live run happens after the merge; until then the write path is checked only by reading it.
