# 0016: Predictive passing shares geometry

Status: accepted; amended by [0017](0017-passing-coordinator.md), which adds a plan lifecycle to the engine for applications without factory rules. Passing geometry belongs in the headless simulation library, and applications own maneuver state and recovery.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

i4sim already predicts head-on encounters, chooses lanes and watches for intrusions. The same calculations can serve other applications. i4sim also combines priority steering's overlap correction and border constraints with acceleration from other solvers during a single update.

## Decision

Kinetra provides conflict timing, ordered lane candidates and a snapshot guard for axis-aligned footprints. Applications choose goals, approve candidates against the world, assign priority among movers and decide when to wait, retry or recover. A shared stateful controller would make factory wait and recovery rules part of the engine contract before they are proven elsewhere.

The four steering solvers keep their public interfaces. Passing provides a target upstream of the solver. Each caller translates arrival intent and supplies explicit limits and tuning. A common solver result would hide information consumers use, including reciprocal feasibility and priority steering's individual forces.

## Consequences

A new consumer can reuse the geometry with its own clock and scale. Each application must integrate maneuver targets and determine which traffic yields. The factory pressure and golden tests exercise the shared geometry through the real integration. [Navigation contract](../engine/navigation.md#predictive-passing) · [Design](../design/2026-11-26-shared-passing.md)
