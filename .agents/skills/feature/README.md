# feature

Use `feature` when you want a change delivered in Kinetra or Kinetra-Lab. You get a design document, verified commits and a pull request that goes through independent review.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| A roadmap item is ready | “Pak het item Skill READMEs op.” | One branch with the seven guides, their design document and the corresponding roadmap update. |
| An engine contract appears broken | “/feature Zoek uit waarom een mislukte Navigation::rebuild de vorige kaart wist.” | A test that reproduces the reported failure before the fix, followed by the verified fix. |
| A Lab feature needs engine support | “/feature Voeg de agent-inspector toe aan i4sim.” | A design for the engine overlay and the Lab content, with linked pull requests in dependency order. |

## When not to use it

Use `roadmap` to save an idea for later. For an existing branch that only needs its checks run, use `verify`.

## How it works

1. The agent reads the roadmap, review lessons, affected code and tests, then records the scope and test plan. It asks you about choices that affect the design or its contracts.
2. It implements the design on a topic branch or stack, checks the diff and opens the pull requests. An independent reviewer checks the result and the author answers the findings.
3. The author can merge an eligible pull request once review and checks are complete. Changes to skills, agent rules, CI or releases, and checks reserved for you, wait for your decision. [Merge rule](../../../AGENTS.md#git)

## Related skills

[Roadmap](../roadmap/README.md) supplies agreed work. [Grilling](../grilling/README.md) settles open choices and [adr](../adr/README.md) records lasting decisions. A design can schedule a [refactor](../refactor/README.md) or call for [module documentation](../doc-module/README.md). [Verify](../verify/README.md) checks the diff. [pr-review](../pr-review/README.md) and [pr-feedback](../pr-feedback/README.md) handle the independent review and the author's response.
