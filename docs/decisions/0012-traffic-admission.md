# 0012: Occupancy-aware traffic admission

Status: accepted. Zone geometry and traffic policy remain application-owned.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

Local avoidance can keep agents separated while sending a crossing agent into an
unintended narrow arm. Applications need explicit admission before conflict zones,
without importing factory grids or gameplay rules into the engine.

## Decision

Add `TrafficGate` to the existing `Kinetra::Simulation` target. It retains FIFO
request order with stable ID tie-breaking and prioritizes observed physical
occupancy over logical cancellation. Multiple occupants prevent new admission.
Applications supply complete request/occupancy snapshots, discover zones, constrain
movement before entry and allow occupants to exit. No timer revokes a grant while
an agent may still be approaching at speed.

An additive eligibility overload lets callers defer requests with blocked exits
without cancelling their FIFO tickets. New grants select the oldest eligible
request. Existing grants and physical occupancy still take precedence; readiness
changes never revoke an approaching grant. Exit geometry and reservations remain
application-owned.

## Consequences

The mechanism is headless and independent of math, JSON and application models.
i4sim initially uses it only for isolated T/+ junctions with single-cell corridor
arms. More complex intersections, exit clearance/reservations, stalled owners and multi-zone
cycles require additional application policy. FIFO progress assumes each admitted
agent eventually clears the zone. Tests cover state transitions and the external
consumer links the public API. The authoritative contract is
[navigation and traffic admission](../engine/navigation.md#traffic-admission).
