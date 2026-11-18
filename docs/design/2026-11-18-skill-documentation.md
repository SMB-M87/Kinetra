# Skill documentation

Status: implemented · Branch: `docs/skill-documentation` · Related: [skills overview](../../.agents/skills/README.md), [skill README template](../documentation.md#skill-readme-template), Kinetra-Lab quality roadmap (documentation cleanup)

Every repository skill gets a README that explains it to people with use cases, next to the `SKILL.md` that instructs the agent. This branch records the rule and the template. The READMEs for the seven existing skills follow on their own branch.

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

- **Written for agents only.** A `SKILL.md` holds discovery frontmatter and imperative steps. A person who wants to know which skill fits a situation, what it will change and what it never does has to read those steps.
- **No examples.** The [skills overview](../../.agents/skills/README.md) gives one line per skill and links `SKILL.md`. It does not show when to pick `feature` rather than `refactor`, or `verify` on its own.
- **No standard.** The [documentation guide](../documentation.md) has templates for module READMEs and pages, but nothing for skills.

## Goals and non-goals

**Goals:**
- A rule that every skill has a `README.md` with use cases, in the documentation guide and in `AGENTS.md`.
- A template for that README.
- The skills overview as the one page that lists all skills.

**Non-goals:**
- Writing the READMEs for the seven existing skills. They follow on their own branch.
- Changing any `SKILL.md`.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Where the explanation lives | A `README.md` in each skill directory, next to `SKILL.md` | Requested by the user on 2026-11-18. `SKILL.md` stays short for the agent's context; people get one page per skill |
| Overview | The existing `.agents/skills/README.md` stays the page that lists all skills | It is already the root page of the skills, and Kinetra-Lab reaches it through its symlink |
| Steps | The README summarizes; `SKILL.md` stays the authoritative procedure | One authoritative place per concept, so the two cannot drift on details |
| Delivery | Rule and template now; the seven READMEs on a follow-up branch | Requested as a documentation and roadmap item; each README can then be reviewed against its skill |

## Design

The [skill README template](../documentation.md#skill-readme-template) has four sections:
- **Use cases:** a table of situation, example request and result.
- **When not to use it:** look-alike situations that belong to another skill.
- **How it works:** the main steps, what is read, changed and asked, and what the skill never does.
- **Related skills:** which skills it calls and which call it.

The README is not part of the agent's instructions, so it adds no context cost. A change that adds or changes a skill updates its README and the overview in the same commit; `AGENTS.md` states this rule with its reason, and the overview repeats it under *Maintenance* for whoever edits a skill.

## Affected areas

- `docs/documentation.md`: a *What goes where* row and the *Skill README template* section.
- `.agents/skills/README.md`: the two-file layout, the maintenance rule, and an open point for the missing READMEs.
- `AGENTS.md`: the rule in *Skills*.
- `docs/README.md`: a question row pointing to the overview.
- `docs/design/README.md`: this document in the index.

## Test plan

Documentation only, so the **Docs** level applies: `python3 tools/check_docs.py` must pass. The template's links are checked by hand, because the checker skips fenced code: from `.agents/skills/<skill>/README.md`, `../README.md` is the overview and `../../../` is the repository root.

## Review

- **Planned versus implemented.** The rule is stated as the standard, and the overview marks the missing READMEs as an open point, as the [style rules](../documentation.md#style-and-maintenance) ask.
- **Duplication.** The template asks for a summary of the steps, not a copy, so `SKILL.md` stays the one procedure.
- **Rule in one place.** A first draft also repeated the same-commit rule at the end of the template; it was removed, because the guide's *Style and maintenance* rules already cover it.
- **Placement in `AGENTS.md`.** The rule and its *Why* sit below the skills table, as the reason ends every other section.
- **Merge overlap.** The Kinetra phase 3 branch will also change the design index (the status of the overlay design). If the edits touch adjacent lines, the branch that merges second gets a small conflict in that table.

## Verification

| Command | Result |
| --- | --- |
| `python3 tools/check_docs.py` | passed |
| Template link paths, by hand | resolve from `.agents/skills/<skill>/README.md` |

## Pull request

**Title:** `docs: require a README with use cases for every skill`

The description is published with the pull request. It covers the rule, the template, the overview, the follow-up branch for the seven READMEs, and the merge order: independent of other Kinetra branches, and no Kinetra-Lab pointer change is needed.

## Open points

- The READMEs for `feature`, `grilling`, `verify`, `doc-module`, `refactor`, `adr` and `pr-feedback` are not written yet. They are planned in the Kinetra-Lab quality roadmap.
