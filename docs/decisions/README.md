# Architecture decisions

ADRs preserve the context and trade-offs behind Kinetra's boundaries: what was decided, which alternatives were rejected and why. They are history. Current behavior is described by the module guides, the [architecture](../architecture.md) and the [engine contracts](../README.md#engine-contracts).

## Table of contents

- [Decision index](#decision-index)
- [Reading and maintenance](#reading-and-maintenance)

[Project overview](../../README.md) · [Documentation index](../README.md)

## Decision index

| ADR | Decision | Status |
| --- | --- | --- |
| [0001](0001-independent-2d-foundation.md) | Independent 2D engine and simulation modules | Historical; the repository boundary is now [0009](0009-core-and-lab.md) |
| [0002](0002-camera-driven-terrain.md) | Seeded procedural terrain residency | Historical; the terrain is application code in Kinetra-Lab ([0009](0009-core-and-lab.md)) |
| [0003](0003-sandbox-navigation-and-population.md) | Sandbox routes and player-active procedural agents | Historical; planner ownership superseded by [0008](0008-public-engine-and-consumers.md) |
| [0004](0004-sandbox-application-boundaries.md) | Separate sandbox session policy from input and rendering | Historical; the sandbox lives in Kinetra-Lab ([0009](0009-core-and-lab.md)) |
| [0005](0005-sandbox-agent-lifecycle.md) | Keep sandbox agent lifecycle state together | Historical; the sandbox lives in Kinetra-Lab ([0009](0009-core-and-lab.md)) |
| [0006](0006-variants-and-dotio-port.md) | Variants and Dotio boundaries | Historical; session ownership amended by [0008](0008-public-engine-and-consumers.md) |
| [0007](0007-module-tests-and-streamed-dotio.md) | Module tests and streamed Dotio world | Historical; Dotio lives in Kinetra-Lab ([0009](0009-core-and-lab.md)) |
| [0008](0008-public-engine-and-consumers.md) | Public engine mechanisms and independent consumers | Accepted; repository placement and packaging superseded by [0009](0009-core-and-lab.md) |
| [0009](0009-core-and-lab.md) | Separate core library from Lab consumers | Accepted; launcher ownership superseded by [0010](0010-generic-launcher-in-core.md) |
| [0010](0010-generic-launcher-in-core.md) | Generic launcher in Kinetra | Accepted; amended by [0014](0014-session-navigation-requests.md) |
| [0011](0011-weighted-navigation-and-priority-steering.md) | Weighted navigation and priority steering | Accepted |
| [0012](0012-traffic-admission.md) | Occupancy-aware traffic admission | Accepted |
| [0013](0013-settings-overlay-in-kinetra.md) | Settings overlay in Kinetra, built from application rows | Accepted |
| [0014](0014-session-navigation-requests.md) | Session navigation requests and configurable launcher bindings | Accepted; amends [0010](0010-generic-launcher-in-core.md) |

*Reading the table:* 0001 to 0007 were written while the applications lived in this repository; their application rules now belong to Kinetra-Lab. [0009](0009-core-and-lab.md) defines the current boundary between Kinetra and Lab.

## Reading and maintenance

- Read an ADR's status line and the decisions that supersede or amend it before applying it to current code.
- Write a new ADR with the `adr` skill when a decision is hard to reverse, surprising without context and the result of a real trade-off. Number it after the highest number here and add a row to the index in the same commit.
- When a new ADR replaces or amends an older one, update the older ADR's status line and its row here. Never rewrite the decision text of an older ADR: it records what was decided then.
