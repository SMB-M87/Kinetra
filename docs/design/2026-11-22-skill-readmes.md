# Skill READMEs

Status: accepted · Branch: `docs/skill-readmes`

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

Pending the review of the guides.

## Verification

Pending the documentation checks.

## Pull request

The description will record the guides, verification and review outcome.
