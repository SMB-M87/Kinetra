# 0017: A passing coordinator for applications without factory rules

Status: accepted. Amends [0016](0016-predictive-passing.md): the engine also offers a stateful plan lifecycle on top of the shared geometry. i4sim keeps its own coordinator.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

[ADR 0016](0016-predictive-passing.md) put conflict timing, lane candidates and the lane guard in the engine and left plan state to applications, because a shared stateful controller would have made factory wait and recovery rules part of the engine contract before another application had used them. Kinetra-Lab then gave Demo's crowds and Dotio's enemies head-on passing. Both need the same lifecycle that i4sim runs: form a plan, drive to the entries, cross, wait for traffic with priority, give up after a timeout, retry later. Written twice in Lab, it would be a third and fourth copy of the same mechanism, with no game rules in it. On 2026-11-27 the user chose to put it in the engine.

## Decision

`Kinetra::Simulation` provides `PassingCoordinator`. Each step the caller supplies its movers (body, route heading, destination, available, stationary, present; an empty slot of a pool is not present, so indices stay stable), the tuning in world units and steps, and a check of a leg against its world. The coordinator owns plans, waits, retries and outcomes; it checks legs against the other movers and the space of older plans, and gives each participant a target. Priority is fixed: a plan yields to movers in no plan and to the participants of older plans.

What stays with the application: which movers are available or stationary, the world check, the steering solver that drives a participant to its target, replanning after a plan ends, the tuning, and any further remedy. Every value is explicit; the coordinator has no defaults.

i4sim keeps its factory coordinator: it also holds traffic controls, corridor recovery and an event log that its golden and pressure suites pin. Moving i4sim onto the engine coordinator is a later choice, once the stall remedies settle whether making way and a motion fallback belong in it.

## Consequences

Demo and Dotio share one tested lifecycle, and another consumer can pass with its own scale and clock. The priority rule and the end conditions are now an engine contract; a consumer that needs another rule asks for an extension instead of copying the coordinator. Movers of any shape take part through their axis-aligned bounds, which is conservative for rotated and round bodies. [Navigation contract](../engine/navigation.md#predictive-passing) · [Design](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-27-shared-passing-mechanism.md)
