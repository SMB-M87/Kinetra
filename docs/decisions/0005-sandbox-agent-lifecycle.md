# 0005: Keep sandbox agent lifecycle state together

## Table of contents

- [Context and preservation contract](#context-and-preservation-contract)
- [Decision](#decision)
- [Alternatives and limits](#alternatives-and-limits)
- [Verification](#verification)

## Context and preservation contract

The earlier [application-boundary refactor](0004-sandbox-application-boundaries.md) deliberately retained scene storage. Agent body IDs, homes, goals, routes, visibility and replan timers were stored in six parallel vectors. Spawning had to append to all six, while streaming, update code, rendering and tests shared their index convention. This distributed one lifecycle invariant across callers.

Keep observable behavior: identical seeded spawns and physics body IDs, persistent exploration population, the 1,200-agent cap, camera-based sleeping, two-second replanning, patrol transitions, route display, command results and existing input bindings. No simulation algorithm or public engine API changes are part of this refactor.

The patrol behavior below records the original preservation contract. Free wandering and post-command transitions now follow [decision 0006](0006-variants-and-dotio-port.md); entity ownership remains unchanged.

## Decision

The modeling context is the sandbox application. Its `Agent` is an autonomous patrol entity whose identity is its scene-owned physics body index. The reusable simulation context continues to know only generic bodies; it does not import sandbox types or patrol rules.

Replace the parallel vectors with one private `vector<Agent>`. Keep each agent's body ID, home, goal, route, active flag and replan timer private. Express lifecycle changes with `retarget`, `invalidate_route`, `update_visibility` and `advance_navigation`. The scene alone creates population entries and owns populated-chunk bookkeeping. Presentation reads a const span of agents and const route spans; it cannot append agents or edit their goals/timers. Spans must not be retained across streaming, which can grow the population; body IDs remain stable.

Retargeting preserves the displayed route until an active tick replans. Streaming invalidates timers without discarding route display. Sleeping agents do not advance navigation state. Reaching an outbound or assigned goal switches to home; reaching home switches to the existing offset patrol point. Failed planning holds position and retains the two-second retry interval. The shared waypoint-consumption rule is owned by the navigation module and used by both player and agents.

This applies Extract Class, Replace Parallel Arrays with Objects, Encapsulate Collection and Move Function to a concrete maintenance problem. No dispatch framework or additional service layer is introduced.

## Alternatives and limits

A public state record would group the arrays but still let callers independently change home, goal and route timers. A separate population manager would add another owner coordinating the same scene and physics lifecycle. A reusable engine agent abstraction would move sandbox-specific patrol policy across the existing boundary. The chosen entity hides the current state transitions without those additional layers.

This is a focused population consistency boundary, not a claim that the entire scene is a fully encapsulated transactional Aggregate. The scene still exposes its physics world and player/debug state for sandbox use. Exceptional allocation failures and failed physics ticks retain the existing recovery limits; no new rollback contract is claimed. Agent storage is transient example code, with no persisted data to migrate. Internal sandbox callers must migrate from parallel vectors to `agents()` and the agent queries; the public simulation/runtime API remains compatible.

Repositories, Domain Events, Event Sourcing, generic identity wrappers and persistence machinery have no current requirement here. Further scene decomposition should follow an observed maintenance problem rather than expand this cleanup.

## Verification

Add lifecycle characterization to [terrain tests](../../examples/demo/tests/terrain_tests.cpp) and run it against the original implementation before moving state. Keep those assertions through the refactor, using behavior operations and read-only observations instead of editing agent collections. Direct agent checks cover dormant navigation, waypoint consumption, retained routes before replanning and unreachable-goal braking. Existing population tests cover seed variation, spawn clearance, persistent body IDs, streaming, sleeping and revisits; [application tests](../../examples/demo/tests/sandbox_application_tests.cpp) cover targeting and same-seed reset.

A local before/after diagnostic compared complete body fields and agent goals, homes, visibility, route timers and waypoints for seeds 42 and 777, initial populations 0, 12 and 240, five view visits per scenario and five fixed ticks per visit. Its outputs matched exactly on this build environment. This is a refactoring check, not a promise of cross-platform bitwise simulation determinism. Durable semantic regression coverage remains in the test suites.

Run headless and runtime builds, both CTest suites and the bounded graphical sandbox smoke run described in the [README](../development.md#checks). Update the [architecture diagrams](../architecture.md) with the agent ownership relationship.
