# Documentation

Start with the architecture map, then follow the subsystem contract relevant to your change. Source and behavioral tests define the detailed API.

## Table of contents

**Navigation:**

[Parent guide](../README.md)

Directory guides: [decisions](decisions/README.md), [engine](engine/README.md).

**On this page:**

- [Design and workflows](#design-and-workflows)
- [Applications](#applications)
- [Decisions](#decisions)

## Design and workflows

| Question | Authoritative page |
| --- | --- |
| Where are public interfaces and implementations? | [Headers](../include/README.md) and [sources](../src/README.md) |
| How are tests organized? | [Tests](../tests/README.md) and [consumer fixture](../tests/consumer/README.md) |
| Where are subsystem contracts indexed? | [Engine contracts](engine/README.md) |
| How are historical decisions organized? | [Decision guide](decisions/README.md) |
| What depends on what? Who owns objects? | [Architecture](architecture.md) |
| How do I build, test and validate a change? | [Development](development.md) |
| What are the geometry and physics contracts? | [Math and physics](engine/math-physics.md) |
| How do pathfinding and steering cooperate? | [Navigation](engine/navigation.md) |
| How do window/input/rendering and sessions work? | [Runtime](engine/runtime.md) |
| How do I register applications and overlays? | [Launcher and overlays](engine/launcher-overlays.md) |
| What rules must agents follow? | [AGENTS](../AGENTS.md) |

## Applications

Application behavior, concrete launcher registration and application tests are documented in [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab).

## Decisions

ADRs record historical context, including applications formerly in this repository. [0009](decisions/0009-core-and-lab.md) defines the current Core/Lab boundary; earlier application decisions do not describe the current core layout.

| ADR | Decision |
| --- | --- |
| [0001](decisions/0001-independent-2d-foundation.md) | Independent 2D foundation |
| [0002](decisions/0002-camera-driven-terrain.md) | Seeded terrain residency |
| [0003](decisions/0003-sandbox-navigation-and-population.md) | Original application-owned planner; ownership superseded by 0008 |
| [0004](decisions/0004-sandbox-application-boundaries.md) | Sandbox policy/input/presentation boundaries |
| [0005](decisions/0005-sandbox-agent-lifecycle.md) | Cohesive agent lifecycle |
| [0006](decisions/0006-variants-and-dotio-port.md) | Original variant/port boundary; session ownership amended by 0008 |
| [0007](decisions/0007-module-tests-and-streamed-dotio.md) | Streamed Dotio window and module tests |
| [0009](decisions/0009-core-and-lab.md) | Core/Lab split and source integration |
| [0010](decisions/0010-generic-launcher-in-core.md) | Generic launcher ownership in Kinetra |
| [0008](decisions/0008-public-engine-and-consumers.md) | Public navigation, optional launcher and external consumers |
