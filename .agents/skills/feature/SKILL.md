---
name: feature
description: Run a change in Kinetra or Kinetra-Lab through the full development cycle (understand, design document, decisions, test plan, implementation, verification, review, documentation, commits, push and pull request, hand-over). Use when starting any code or documentation change that will be committed, or when the user says /feature, "nieuwe feature", "pak dit op", "fix deze bug".
argument-hint: "<topic or problem description>"
---

# Feature: the full development cycle

This skill carries out [docs/workflow.md](../../../docs/workflow.md). (In Kinetra-Lab, the path is `external/Kinetra/docs/workflow.md`.) Read that page first; this skill only adds the concrete steps. Talk to the user in Dutch; write code, docs and commits in English.

## 1. Understand

1. Determine the repositories involved: Kinetra only, Lab only, or both (engine mechanism plus application use).
2. In each repository: `git status --short` and `git branch --show-current`. Report unrelated changes and leave them alone. If the current branch is not `main` and not an obvious branch for this topic, ask the user before continuing.
3. Read `AGENTS.md`, the routing-table pages for this task, the module README, and the code and tests involved. Check `docs/roadmap.md` of each repository for an item on this topic: it carries the agreed scope and questions. Read `docs/review-lessons.md` of each repository: mistakes earlier reviews found, which this change must not repeat. For a broad survey, dispatch a read-only sub-agent with a narrow brief.

## 2. Branch and design document

1. Decide whether the goal is one pull request or a stack. When it splits into steps that can each be reviewed on their own, plan a [stack](../../../docs/workflow.md#stacked-pull-requests): one pull request per step, each based on the step below. Propose the cut into steps to the user with the design.
2. Create a branch in each repository involved: `git switch -c <type>/<topic>` from up-to-date `main`, or from the branch of the step below in a stack (types in the [workflow](../../../docs/workflow.md#branches-and-commits)).
3. Copy the [template](../../../docs/design/README.md#template) to `docs/design/YYYY-MM-DD-<topic>.md` and add it to the index with status `draft`. Skip this for a docs-only change (only Markdown, `docs/` or `.agents/`): its pull request description takes the problem, the decisions and the verification, and whatever steps 2.4 to 7 write into the design document goes there too. A change across both repositories gets one document, in the repository with most of the change; the other links it. A stack shares the document of its first branch; list the steps with their branches in it.
4. Fill in Problem, with evidence such as `path:line` and measurements, plus Goals and non-goals, Design and Affected areas.
5. If the change has a roadmap item, move it to **Now** with the branch and the design document, if it has one (`roadmap` skill). Non-goals that the user still wants become roadmap items.

## 3. Decisions

- List the open decisions. Are any of them load-bearing? With one, ask it in a form. With several, use the `grilling` skill.
- Record each outcome in the **Decisions** table. Set the status to `accepted` once all load-bearing decisions are settled.
- If a decision meets the ADR criteria, use the `adr` skill.

## 4. Test plan

Write the **Test plan** section before coding: each new or changed behavior, the test that proves it, and the verification level. For a bug, write the failing reproduction test first and run it to see it fail. For a refactor, confirm that tests pin the behavior; add characterization tests first if they don't.

## 5. Implement in commits

- Work in small steps. After each step, verify at the targeted level (the `verify` skill) when code changed.
- Commit each logical step: `git add <explicit paths>` and `git commit -m "type(scope): summary"`. Refactor and behavior-change commits stay separate.
- Write `///` summaries and *why*-comments as you go ([documentation guide](../../../docs/documentation.md#code-comments)).
- Note unrelated bugs under *Open points*; do not fix them here.

## 6. Verify

Run the `verify` skill on the whole branch diff. A branch with code changes ends at the **Full** level. Record commands and results in the **Verification** section.

## 7. Review

Review `git diff main...HEAD` with `/code-review` (Claude Code) or the global `code-review` skill (Codex), and check the list in the [workflow](../../../docs/workflow.md#6-review). For every rule, procedure or term the branch changes, search for the old wording, its key words and the names of its parts (`python3 tools/check_copies.py --term '<old wording>' --term '<key word>' --term '<part>'`, in Kinetra-Lab `python3 external/Kinetra/tools/check_copies.py`, which also covers `external/Kinetra`; it searches case-insensitively, and lists the CI jobs, CMake options, presets, flags and inline names the diff removes and where they are still named), read every hit, and change every copy in the same commit, or in the fix commit when this review finds a missed copy ([rule](../../../AGENTS.md#repository-documentation), which says what the parts are). Fix the findings in new commits, re-verify, and record findings and resolutions in **Review**.

## 8. Document

- Update the module README (with the `doc-module` skill if it is thin), contract pages, diagrams and indexes.
- Set the design document's status to `implemented`, and fill **Pull request** with the title and description. A docs-only change has no design document; write its title and description in step 9.3.
- Update the roadmap with the `roadmap` skill: remove the item this branch finishes, and add the open points someone intends to act on as items.
- Run the docs check, then make the final commit.

## 9. Push and open pull requests

Follow [workflow step 8](../../../docs/workflow.md#8-commit-push-and-open-a-pull-request):

1. Check that `git status --short` shows no stray files. Run a final `git log --oneline main..HEAD` and make sure every commit belongs to this change.
2. Push each branch: Kinetra before Lab. Run `git push -u origin <branch>`; if SSH is unavailable, use the HTTPS fallback from `AGENTS.md`.
3. Write the description from the design document's **Pull request** section, or for a docs-only change directly, to a file in the session scratchpad. It covers Summary, Changes, Design, Verification, Review notes, Dependencies and merge order, and Follow-up.
4. Run `gh pr create --repo <owner>/<repo> --base main --head <branch> --title "<type(scope): summary>" --body-file <file>`; for a step of a stack, use the branch below as `--base`, open the pull requests bottom-up, and start each description with the step's place in the stack and "merge after #<n>". For a Lab pull request with a pointer change, link the Kinetra pull request and state that it merges first, with a merge commit. While the Kinetra pull request is in review and its branch moves, follow it with a pointer commit on the Lab branch. Once both have merged, move the Lab pointer to the Kinetra merge commit in a separate pointer-only pull request, which needs no agent review; merge it when its CI passes.
5. Run `gh pr view <number> --json url,mergeable` and report conflicts. Do not merge yet: step 9.8 merges every pull request, a docs-only one included.
6. Wait for the pull request's checks (`gh pr checks <number>`): they are the Full level. Fix a red check on the branch before handing over; report checks that are still running as pending.
7. Once every check is green, start the agent review, except for a docs-only or pointer-only pull request, which goes straight to step 9.8. Run it on the most capable model the agent offers, at the effort and code-review depth that the [pr-review levels](../pr-review/SKILL.md#levels) give for this diff. Start it where it does not share your context: a new session or task, if the agent can create one; else a fresh sub-agent that gets only the link and the depth; else ask the user to start the `pr-review` skill in a new session. The pull request waits for its conclusion.
8. Follow the pull request: subscribe to its activity where the agent can, so CI results and review comments arrive in this session; otherwise check it when the user asks. Answer the agent review's findings, and other review comments, with `pr-feedback` as they come. When nothing is open and the [merge rule](../../../AGENTS.md#git) allows it, merge; otherwise the pull request waits for the user.

## 10. Hand over (in Dutch)

- What changed and why, in a few lines.
- The pull-request links, and the commits per repository.
- Verification commands and results, and anything not run, with the reason.
- Remaining risks and open points, and the roadmap items added or removed.
- The merge order: Kinetra before Lab, and each stack bottom-up.
- The agent review's conclusion, the points still open for the user, and whether you merged or the pull request waits for the user (and why).
