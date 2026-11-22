# adr

Use `adr` to preserve the reason for an architectural choice that would be costly to reverse and surprising without its context. The result is a numbered decision record explaining the alternatives and consequences.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| Ownership of the proposed inspector overlay has been settled | “/adr Leg vast waarom Kinetra de inspector tekent en Lab de agent kiest.” | An engine ADR explaining the ownership boundary, the alternative considered and what applications must supply. |
| A Lab file format needs a lasting compatibility policy | “/adr Leg onze compatibiliteitskeuze voor i4sim-blueprints vast.” | A Lab ADR linked from the design, with the cost of the alternative migration policy. |
| A new decision replaces an earlier one | “/adr Documenteer de afgesproken wijziging van de traffic-admissionregel.” | A new decision linked to the earlier ADR, with its predecessor's status and the index updated. |

## When not to use it

Use `grilling` while the choice is still open. A routine choice with an obvious rationale fits in the design document's Decisions table.

## How it works

1. The agent checks the cost of reversal, the need for context and the alternatives you actually considered. If the decision falls short of the ADR criteria, it points you to the design table.
2. For a qualifying decision it writes the next numbered record in the repository that owns the contract, then connects it to the design, index and relevant architecture or contract page.
3. It preserves the history of amended or superseded decisions and runs the documentation check.

## Related skills

[Feature](../feature/README.md) and [grilling](../grilling/README.md) can lead to an ADR once the choice and its rationale are known.
