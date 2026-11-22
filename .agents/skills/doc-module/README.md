# doc-module

Use `doc-module` when a contributor needs to understand or change one module. It produces a module README grounded in the implementation and contract comments beside the declarations that callers use.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| A newcomer needs to trace one i4sim tick | “/doc-module labs/i4sim — leg de simulatiestap uit.” | A guide that traces the tick through the real types, states the invariants and points to tests. |
| Callers need the simulation API's units and preconditions | “/doc-module src/simulation — documenteer de publieke contracten.” | A simulation guide and header summaries covering units, inputs and outcomes supported by the implementation. |
| The launcher guide mostly lists directories | “/doc-module launcher — beschrijf hoe een sessie start.” | A module README explaining application registration and session startup, with links repaired when navigation-only pages are folded in. |

## When not to use it

Use the skill README template for a guide to an agent skill. Use `feature` when implementing a change to a module's behavior.

## How it works

1. The agent reads the module headers, implementation, tests and existing decisions. When the sources leave a design choice unexplained, the agent asks you or records an open question.
2. It documents public contracts in the headers and explains the data flow, diagrams, change recipes and test coverage in the module README. A suspected defect becomes an open point.
3. It checks names and diagrams against the implementation, runs the Docs check and commits the documentation.

## Related skills

[Feature](../feature/README.md) can call this skill when a change exposes a thin module guide. [Verify](../verify/README.md) checks the resulting documentation and comment edits.
