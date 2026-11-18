---
name: feature
description: Run a change in Kinetra or Kinetra-Lab through the full development cycle (understand, design document, decisions, test plan, implementation, verification, review, documentation, commits, hand-over). Use when starting any code or documentation change that will be committed, or when the user says /feature, "nieuwe feature", "pak dit op", "fix deze bug".
argument-hint: "<topic or problem description>"
---

# Feature: the full development cycle

This skill carries out [docs/workflow.md](../../../docs/workflow.md). (In Kinetra-Lab, the path is `external/Kinetra/docs/workflow.md`.) Read that page first; this skill only adds the concrete steps. Talk to the user in Dutch; write code, docs and commits in English.

## 1. Understand

1. Determine the repositories involved: Kinetra only, Lab only, or both (engine mechanism plus application use).
2. In each repository: `git status --short` and `git branch --show-current`. Report unrelated changes and leave them alone. If the current branch is not `main` and not an obvious branch for this topic, ask the user before continuing.
3. Read `AGENTS.md`, the routing-table pages for this task, the module README, and the code and tests involved. For a broad survey, dispatch a read-only sub-agent with a narrow brief.

## 2. Branch and design document

1. Create a branch in each repository involved: `git switch -c <type>/<topic>` (types in the [workflow](../../../docs/workflow.md#branches-and-commits)).
2. Copy the [template](../../../docs/design/README.md#template) to `docs/design/YYYY-MM-DD-<topic>.md` and add it to the index with status `draft`.
3. Fill in Problem, with evidence such as `path:line` and measurements, plus Goals and non-goals, Design and Affected areas.

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

Review `git diff main...HEAD` with `/code-review` (Claude Code) or the global `code-review` skill (Codex), and check the list in the [workflow](../../../docs/workflow.md#6-review). Fix the findings in new commits, re-verify, and record findings and resolutions in **Review**.

## 8. Document

- Update the module README (with the `doc-module` skill if it is thin), contract pages, diagrams and indexes.
- Set the design document's status to `implemented`, and fill **Pull request** with a title and description the user can paste.
- Run the docs check, then make the final commit.

## 9. Hand over (in Dutch)

- What changed and why, in a few lines; the branch and its commits per repository.
- Verification commands and results, and anything not run, with the reason.
- Remaining risks and open points.
- What the user must push, in order: Kinetra before Lab when the pointer changed. Agents never push or open pull requests.
