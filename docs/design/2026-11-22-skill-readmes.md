# Skill READMEs

Status: implemented · Branch: `docs/skill-readmes`

The skills overview should let a contributor choose a skill by seeing the request they could make and what they would get back.

## Table of contents

- [Problem and scope](#problem-and-scope)
- [Decisions](#decisions)
- [Design and affected areas](#design-and-affected-areas)
- [Test plan](#test-plan)
- [Review](#review)
- [Verification](#verification)
- [Pull request](#pull-request)

[Design index](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem and scope

The overview gives readers the agent procedure for seven skills. Only `roadmap` and `pr-review` already have user guides. The roadmap item calls for the other seven guides on one Kinetra branch. [Original skill documentation design](2026-11-18-skill-documentation.md)

## Decisions

| Decision | Choice | Reason |
| --- | --- | --- |
| Example requests | Give Dutch prompts in English guides | Contributors can use the prompts directly, as in the two existing guides. |
| Procedure details | Link each guide to its sibling instructions | Readers can check the exact procedure and commands at their source. |

## Design and affected areas

Use the existing skill README template. Link every guide from the skills overview, remove its temporary open point and update references to that section. The seven guides describe the procedures shipped on this branch. [Skill README template](../documentation.md#skill-readme-template)

## Test plan

Read each guide against its sibling instructions, check the overview links for every skill and run `python3 tools/check_docs.py` and `git diff --check`. This change qualifies for Docs verification.

## Review

Compared all seven guides with their sibling procedures, including who agrees to a refactoring plan and who resolves review threads or merges. Updated a historical reference to the retired open point in the design for stacked pull requests.

## Verification

- `python3 tools/check_docs.py` resolved all relative links and heading anchors in 84 Markdown files.
- The overview linked to all nine skill READMEs. All seven new guides had the template sections, a link to the sibling instructions and three example requests with results.
- `git diff --check` passed for the branch.

## Pull request

**Title** — docs(skills): add the seven missing skill READMEs

**Summary.** Contributors can choose any repository skill from the overview and see concrete requests and expected results before running it.

**Changes.** Add seven guides, link them from the overview and close the Skill READMEs roadmap item.

**Design.** [Skill READMEs](2026-11-22-skill-readmes.md)

**Verification.** The documentation check passed for 84 files, the overview covers all nine skills and each new guide has three use cases.

**Review notes.** The examples reflect the current workflow, including independent review and the author's role in handling feedback.

**Dependencies and merge order.** One Kinetra pull request. Changes under `.agents/skills/` require the user's merge decision.

**Follow-up.** None.
