# Documentation

Where each question about Kinetra is answered. Module guides explain the code they sit next to; the pages in this directory cover what spans modules: architecture, engine contracts, decisions, design documents and the way of working.

## Table of contents

- [Modules](#modules)
- [Engine contracts](#engine-contracts)
- [Design and workflows](#design-and-workflows)
- [Applications](#applications)
- [Decisions](#decisions)

[Project overview](../README.md)

Directory guides: [decisions](decisions/README.md), [design](design/README.md), [media](media/README.md).

## Modules

| Question | Authoritative page |
| --- | --- |
| What does the headless simulation contain, and how does it work? | [Simulation](../src/simulation/README.md) |
| How do the window, sessions, overlays and launcher work? | [Runtime and launcher](../src/runtime/README.md) |
| Which test suites exist, and how does the external consumer work? | [Core tests](../tests/README.md) |
| What exactly does one function promise? | The `///` summary in its header in [include/kinetra](../include/kinetra/) |
| What depends on what? Who owns objects? | [Architecture](architecture.md) |

## Engine contracts

The detailed behavior of each subsystem: units, limits, edge cases and the reasons behind them.

| Question | Authoritative page |
| --- | --- |
| What are the geometry and physics contracts? | [Math and physics](engine/math-physics.md) |
| How do pathfinding, steering, avoidance and traffic admission cooperate? | [Navigation](engine/navigation.md) |
| How do window, input, rendering and timing work? | [Runtime](engine/runtime.md) |
| How do sessions, the launcher and the overlays work? | [Launcher, sessions and overlays](engine/launcher-overlays.md) |
| What do the launcher and the overlays look like, and when are their GIFs refreshed? | [Shared UI in GIFs](media/README.md) |

## Design and workflows

| Question | Authoritative page |
| --- | --- |
| How does a change move from idea to commit? | [Workflow](workflow.md) |
| How is C++ and CMake written here? | [Conventions](conventions.md) |
| What do I test, and which checks does a change need? | [Testing](testing.md) |
| How do I build with presets, set up clangd/clang-tidy, or run graphics or consumer checks? | [Development](development.md) |
| What does CI check on a pull request? | [Testing: continuous integration](testing.md#continuous-integration) |
| How are READMEs, diagrams and `///` comments written? | [Documentation guide](documentation.md) |
| What is being changed now, and why? | The *Now* section of the [roadmap](roadmap.md), and the design document its item links; the [design index](design/README.md) is for a search by topic |
| What is planned next, and which ideas are waiting? | [Roadmap](roadmap.md) |
| What rules must agents follow? | [AGENTS](../AGENTS.md) and [agent skills](../.agents/skills/README.md) |
| Which mistakes did reviews find before? | [Review lessons](review-lessons.md) |
| Which skill fits my situation, and what will it do? | [Skills overview](../.agents/skills/README.md) and each skill's README ([template](documentation.md#skill-readme-template)) |
| Which development scripts exist? | [Tools](../tools/README.md) |

## Applications

Application behavior, concrete launcher registration and application tests are documented in [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab).

## Decisions

Why a boundary exists is recorded in an ADR. The [decision index](decisions/README.md#decision-index) lists every ADR with its status and the decisions that superseded or amended it.
