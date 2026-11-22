# refactor

Use `refactor` for a structural change that preserves observable behavior. You get a plan to approve, evidence of the starting behavior and a sequence of small, tested commits.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| An i4sim method mixes several stages of a tick | “/refactor Splits de stappen in de i4sim-simulatie op.” | Named extractions checked against the same tick results after each step. |
| You want to reorganize the Demo crowd logic | “/refactor Ruim de crowd-update op en behoud het gedrag bij dezelfde seed.” | A passing baseline and, where coverage is missing, a separate characterization-test commit before the restructuring. |
| A presentation helper is hard to exercise | “/refactor Maak de tekenfunctie van de launcher leesbaarder.” | A plan limited to compiler-checked mechanical steps where tests are impractical, with a suggested way to test that code later. |

## When not to use it

Use `feature` when changing the public navigation API, an i4sim blueprint format or simulation behavior. A quick tidy-up of a diff rarely needs a structural refactoring plan.

## How it works

1. The agent identifies concrete problems in the requested files or the branch diff and checks which tests and public contracts constrain the change.
2. You see the proposed refactorings in order and agree on the plan before edits. The agent establishes the baseline before restructuring.
3. Each step gets its own build, checks and commit. A failing step is reduced. Bugs found along the way are reported separately.

## Related skills

[Feature](../feature/README.md) can schedule a refactor in a design document. [Verify](../verify/README.md) checks each step against the existing behavior.
