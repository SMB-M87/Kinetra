# 0011: Weighted navigation and priority steering

Status: accepted. Application policy remains in consumers.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

Applications need rectangular-cell routing with configurable traversal costs and axis-aligned body steering with safety priorities. These are reusable mechanisms, while goals, cost selection, scheduling and configuration interpretation are application policy. Existing consumers have separately tuned steering and must retain behavior.

## Decision

Add `WeightedGrid` and `priority_steer` to `Kinetra::Simulation`. Keep `Navigation` and `steer` compatible. WeightedGrid owns bounded grid topology, occupancy costs, A* and free-space borders; applications choose weights and when to replan. Priority steering exposes component forces for inspection and returns bounded acceleration without integration or task decisions. Applications may reuse Navigation for geometric recovery around actual bodies.

Runtime Input adds independent right-button edges/held state with focus-loss cleanup. LaunchMenu accepts an optional heading and status message, allowing application-owned catalogs to share pagination, picking and rendering without importing configuration formats or domain-specific names into the engine.

## Consequences

The headless engine gains no JSON, SDL or application dependency. No target proliferation is required. Timing, units, resource allocation, scheduling, retries and metrics remain application-owned. Two steering modes are explicit because their geometry, time units and priorities differ; they are not silently substituted for existing consumers. New APIs are covered by behavioral tests and the separate public consumer. Port attribution is preserved in Kinetra-Lab's [i4sim license](https://github.com/SMB-M87/Kinetra-Lab/blob/main/labs/i4sim/LICENSE); the engine-side copy was removed when i4sim moved to Lab.
