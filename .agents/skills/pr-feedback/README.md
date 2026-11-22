# pr-feedback

Use `pr-feedback` in the author's session to act on review comments on an open pull request. Reviewers get a reply on each point with the fix, the answer or the evidence for a disagreement.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| A collaborator found a broken link in a skill guide | “/pr-feedback Verwerk de opmerking over de link in de README.” | A correction on the same branch, a Docs check and a reply identifying the commit. |
| An agent reviewer questions a navigation contract | “/pr-feedback Beantwoord de review over Navigation::rebuild.” | A fix or an argument backed by code, a test or the documented contract. A disagreement still open after two rounds goes to you. |
| Feedback affects the bottom of a PR stack | “/pr-feedback Verwerk de feedback op de onderste PR van de stack.” | Verified commits on that branch, then merges into the dependent branches so each review keeps its history. |

## When not to use it

Use `pr-review` to inspect a pull request as its independent reviewer. Use `feature` for a new change that has no open pull request.

## How it works

1. The author reads unresolved feedback on the specified pull request or the current branches. Feedback from outside the owner and collaborators goes to you for a decision.
2. The author fixes or answers each point, records the round in the design and adds new lessons proposed by the agent review. It verifies and pushes the commits and replies in the original threads.
3. The reviewer resolves accepted threads. The author checks the current review conclusion, CI and merge conditions. Pull requests reserved for you wait for your decision. [Merge rule](../../../AGENTS.md#git)

## Related skills

[Feature](../feature/README.md) starts an independent [pr-review](../pr-review/README.md) once the pull request's CI is green. The author processes that review immediately with this skill and uses [verify](../verify/README.md) for the resulting changes.
