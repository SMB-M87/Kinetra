# grilling

Use `grilling` when several plausible answers would lead to different designs. You get the decisions, the reasons for them and a plan you can confirm before implementation.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| The scope of 3D support is open | “/grilling Willen we in Kinetra alleen 3D tekenen of ook in 3D simuleren?” | A round on scope before questions about the renderer and simulation interfaces that depend on it. |
| Engine and application ownership is unclear | “/grilling Wie kiest de agent voor de inspector in i4sim?” | The existing overlay boundary as evidence and a recorded choice about who owns selection. |
| A blueprint format change has compatibility costs | “/grilling Hoe moeten bestaande i4sim-blueprints met de nieuwe velden omgaan?” | Options with migration consequences and the agreed compatibility requirement in the design. |

## When not to use it

Use a repository lookup for a factual question such as which target owns the launcher. Use `roadmap` to collect an idea that only needs a place in the backlog.

## How it works

1. The agent checks the repository first and lists the assumptions it could resolve, with sources you can correct.
2. Each form asks about decisions whose prerequisites are settled. It explains the cost of each option and uses your answers to choose the next round. Reversible details get recommendations.
3. The agreed choices go into the design document, or for a docs-only change into the pull request description, kept in a scratch plan until either exists. You confirm the shared understanding before implementation.

## Related skills

[Feature](../feature/README.md) uses this skill during design. [Roadmap](../roadmap/README.md) can call it to shape an idea and can hold deferred work. A lasting architectural trade-off can lead to an [ADR](../adr/README.md).
