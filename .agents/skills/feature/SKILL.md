---
name: feature
description: Run a change in Kinetra or Kinetra-Lab through the full development cycle (understand, design document, decisions, test plan, implementation, verification, review, documentation, commits, push and pull request, hand-over). Use when starting any code or documentation change that will be committed, or when the user says /feature, "nieuwe feature", "pak dit op", "fix deze bug".
argument-hint: "<topic or problem description>"
---

# Feature: the full development cycle

This skill carries out [docs/workflow.md](../../../docs/workflow.md). (In Kinetra-Lab, the path is `external/Kinetra/docs/workflow.md`.) Each step links its section of the workflow: read that section when you reach the step, not the whole page; this skill only adds the concrete steps. Talk to the user in Dutch; write code, docs and commits in English.

## 1. Understand

1. Determine the repositories involved: Kinetra only, Lab only, or both (engine mechanism plus application use).
2. In each repository: `git status --short` and `git branch --show-current`. Report unrelated changes and leave them alone. If the current branch is not `main` and not an obvious branch for this topic, ask the user before continuing.
3. Read `AGENTS.md`, the sections its routing table names for this task, the module README, and the code and tests involved. Search `docs/roadmap.md` of each repository for the topic (`grep -n -i '<key word>' docs/roadmap.md`, with each key word) and read the items it finds: they carry the agreed scope and questions. Do not read a roadmap through for this. Read `docs/review-lessons.md` of each repository: mistakes earlier reviews found, which this change must not repeat. For a broad survey, dispatch a read-only sub-agent with a narrow brief.

## 2. Branch and design document

1. Decide whether the goal is one pull request or a stack. When it splits into steps that can each be reviewed on their own, plan a [stack](../../../docs/workflow.md#stacked-pull-requests): one pull request per step, each based on the step below. Propose the cut into steps to the user with the design.
2. Create a branch in each repository involved: `git switch -c <type>/<topic>` from up-to-date `main`, or from the branch of the step below in a stack (types in the [workflow](../../../docs/workflow.md#branches-and-commits)).
3. Copy the [template](../../../docs/design/README.md#template) to `docs/design/YYYY-MM-DD-<topic>.md`, reading only that section, and add a row with status `draft` at the top of the index: insert it below the index table's header (`grep -n '^| Date' docs/design/README.md`) without reading the rows. Skip this for a docs-only change (only Markdown, `docs/` or `.agents/`): its pull request description takes the problem, the decisions and the verification, and whatever steps 2.4 to 7 write into the design document goes there too. A change across both repositories gets one document, in the repository with most of the change; the other links it. A stack shares the document of its first branch; list the steps with their branches in it.
4. Fill in Problem, with evidence such as `path:line` and measurements, plus Goals and non-goals, Design and Affected areas.
5. If the change has a roadmap item, move it to **Now** with the branch and the design document, if it has one (`roadmap` skill). Non-goals that the user still wants become roadmap items.

## 3. Decisions

- List the open decisions. Are any of them load-bearing? With one, ask it in a form. With several, use the `grilling` skill.
- Record each outcome in the **Decisions** table. Set the status to `accepted` once all load-bearing decisions are settled.
- If a decision meets the ADR criteria, use the `adr` skill.

## 4. Test plan

Before coding, map each goal to its affected scenarios and the checks that prove them. Include ordinary use, relevant edges and failures, state transitions and existing behavior at risk. Choose existing coverage and add focused regressions for gaps. Follow the [test rules](../../../docs/testing.md#rules-for-tests) and record the failure and passing result for new critical cases.

## 5. Implement in commits

- Work in small steps. After each step, verify at the targeted level (the `verify` skill) when code changed.
- Commit each logical step: `git add <explicit paths>` and `git commit -m "type(scope): summary"`. Refactor and behavior-change commits stay separate.
- Write `///` summaries and *why*-comments as you go ([documentation guide](../../../docs/documentation.md#code-comments)).
- Note unrelated bugs under *Open points*; do not fix them here.

## 6. Verify

Run the `verify` skill on the whole branch diff. A branch with code changes ends at the **Full** level. Check each goal of the design document against a check or run as wide as its wording, and narrow the wording where no such check ran ([rule](../../../AGENTS.md#verification-by-impact)). Record commands and results in the **Verification** section.

## 7. Review

Challenge the implementation and its tests as you work. Look for missing scenarios and assertions that would pass with the wrong behavior. Fix findings in this session, rerun affected checks and record the evidence in **Verification** and **Review**.

## 8. Document

- Update the module README (with the `doc-module` skill if it is thin), contract pages, diagrams and indexes.
- Set the design document's status to `implemented`, and fill **Pull request** with the title and description. For a docs-only change, draft the title and description in the session scratchpad now.
- Update the roadmap with the `roadmap` skill: remove the item this branch finishes, and add the open points someone intends to act on as items.
- Run the docs check. Review the complete diff against the actual pull request base and check the draft description using the [workflow checklist](../../../docs/workflow.md#6-review). Fix findings and rerun checks for the affected parts before making the final commit.

## 9. Push and open pull requests

Follow [workflow step 8](../../../docs/workflow.md#8-commit-push-and-open-a-pull-request):

1. Check that `git status --short` shows no stray files. Run a final `git log --oneline main..HEAD` and make sure every commit belongs to this change.
2. Push each branch: Kinetra before Lab. Run `git push -u origin <branch>`; if SSH is unavailable, use the HTTPS fallback from `AGENTS.md`.
3. Write the description from the design document's **Pull request** section, or for a docs-only change directly, to a file in the session scratchpad. It covers Summary, Changes, Design, Verification, Review notes, Dependencies and merge order, and Follow-up.
4. Run `gh pr create --repo <owner>/<repo> --base main --head <branch> --title "<type(scope): summary>" --body-file <file>`; for a step of a stack, use the branch below as `--base`, open the pull requests bottom-up, and start each description with the step's place in the stack and "merge after #<n>". For a Lab pull request with a pointer change, link the Kinetra pull request and state that it merges first, with a merge commit. While the Kinetra pull request is in review and its branch moves, follow it with a pointer commit on the Lab branch. Once both have merged, Kinetra's `lab-pointer` workflow opens the pointer-only pull request to the Kinetra merge commit, within the hour, and GitHub merges it when its CI passes. Check that it did; open and merge one by hand only when the workflow failed.
5. Check mergeability with `gh pr view <number> --json url,mergeable`.
6. Follow the configured checks for the current head (`gh pr checks <number>`). Fix failures in this session and group related fixes before pushing again. A code change reaches Full verification when the required checks are green.
7. Process actual review feedback with `pr-feedback`. An independent `pr-review` is available when the user requests it.
8. Merge when the [merge rule](../../../AGENTS.md#git) allows it.

## 10. Hand over (in Dutch)

- What changed and why, in a few lines.
- The pull-request links, and the commits per repository.
- Verification commands and results, and anything not run, with the reason.
- Remaining risks and open points, and the roadmap items added or removed.
- The merge order: Kinetra before Lab, and each stack bottom-up.
- The author’s review result, remaining limits and the merge status.
