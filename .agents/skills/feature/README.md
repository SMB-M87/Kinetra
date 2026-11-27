# feature

Use `feature` when you want a change delivered in Kinetra or Kinetra-Lab. The implementing session plans the scenarios, provides regression evidence, and reviews its final diff before pushing. Docs-only changes use the pull request description as the record.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| A roadmap item is ready | “Pak het item Skill READMEs op.” | One branch with the guides, the roadmap update and the author’s checks recorded in the pull request. |
| An engine contract appears broken | “/feature Zoek uit waarom een mislukte Navigation::rebuild de vorige kaart wist.” | A test that reproduces the reported failure before the fix, followed by the verified fix. |
| A Lab feature needs engine support | “/feature Voeg de agent-inspector toe aan i4sim.” | A design for the engine overlay and the Lab content, with linked pull requests in dependency order. |

## When not to use it

Use `roadmap` to save an idea for later. For an existing branch that only needs its checks run, use `verify`.

## How it works

1. The agent reads the roadmap, review lessons, affected code and tests, then records the scope and test plan. It asks you about choices that affect the design or its contracts.
2. The author adds tests for uncovered scenarios and shows that the critical cases fail with the wrong behavior. It checks the final diff, including docs and verification claims, fixes findings and pushes once the local checks pass.
3. GitHub runs the required checks. The author merges under the [merge rule](../../../AGENTS.md#git). You can request an independent review or check the result after the merge.
4. In Kinetra-Lab, Kinetra’s `lab-pointer` workflow opens a separate pull request to move the submodule pointer to the Kinetra merge commit, and GitHub merges that update after CI passes.

## Related skills

[Roadmap](../roadmap/README.md) supplies agreed work. [Grilling](../grilling/README.md) settles open choices and [adr](../adr/README.md) records lasting decisions. [Verify](../verify/README.md) checks the diff. [pr-review](../pr-review/README.md) provides an independent review on request, and [pr-feedback](../pr-feedback/README.md) handles feedback already posted on GitHub.
