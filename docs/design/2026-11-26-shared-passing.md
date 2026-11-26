# Shared predictive passing

Status: implemented · Branch: `refactor/shared-passing` · Related: [Kinetra roadmap](../roadmap.md), [Lab roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md#later)

Extract the geometry used by i4sim's guarded predictive passing into the headless simulation library. i4sim is the first consumer and keeps its existing plan transitions, route restoration and traffic rules. [ADR 0016](../decisions/0016-predictive-passing.md)

## Table of contents

- [Problem](#problem)
- [Goals](#goals)
- [Decisions](#decisions)
- [Design](#design)
- [Affected areas](#affected-areas)
- [Test plan](#test-plan)
- [Verification](#verification)
- [Review](#review)
- [Pull request](#pull-request)
- [Open points](#open-points)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem

i4sim's predictive passing predicts a head-on conflict, searches for room for two lanes and rechecks those lanes while the pair passes. Those calculations currently live beside factory-specific rules, so other simulations cannot use the tested mechanism without adopting factory policy. The priority and tangent steering APIs also supply i4sim-sized defaults to external consumers.

## Goals

Make the existing passing geometry available to headless consumers, retain the factory's pressure results and require application steering parameters at the call site. Application integrations beyond i4sim and the ownership of stall remedies are follow-up work.

## Decisions

| Decision | Choice | Reason |
| --- | --- | --- |
| Passing boundary | Share conflict prediction, ordered lane candidates and geometric obstruction checks. The application owns plan state and selects which neighbors have priority. | The same geometry can support different traffic policies and fallbacks. |
| Shape and time | Use axis-aligned footprints and one consistent caller time unit. Geometry, prediction horizon and sampling cadence are explicit. | This preserves the tested i4sim calculations and allows the caller to choose its scale. |
| Steering interface | Share passing motion intent through an entry, exit or braking target and explicit motion limits. Keep the four solver interfaces distinct. | The solvers have different footprints, inputs and diagnostics. i4sim combines priority overlap and border corrections with other solvers in one tick. |
| Steering tuning | Require explicit priority and tangent settings and supply factory values in i4sim. | A caller's geometry and time scale determine useful limits and distances. |

## Design

`Kinetra::Simulation` provides head-on conflict timing, deterministic candidate lanes and a lane guard. A candidate contains an entry and an exit for each participant. i4sim checks each candidate against its floor, zones, movers and reservations in order. On each tick the guard classifies an observed footprint as clear, crossing or blocking. i4sim owns waiting hysteresis, priority, cancellation, retries and events.

The common input to steering stays a target and its arrival intent. Legacy steering computes acceleration from obstacles and a target. Priority steering also reports diagnostic forces and constrains borders. Tangent steering uses a spring response. Reciprocal avoidance takes a preferred velocity and reports feasibility as well as velocity and acceleration. A single solver selector would hide the composition that i4sim needs.

## Affected areas

The [passing API](../../include/kinetra/simulation/passing.h) and rectangular sweep helpers are part of `Kinetra::Simulation`. The priority and tangent steering declarations, the engine test suite and the external consumer use explicit settings. Lab adapts i4sim's formation and guard and supplies its existing motion tuning.

## Test plan

- Engine tests verify closest approach, candidate geometry and order, insufficient room and goal distance, and guard behavior for stationary, crossing, oncoming and trailing neighbors.
- The external consumer links and calls the engine passing API and supplies steering settings explicitly.
- i4sim's existing pressure, simulation and golden suites exercise the migrated geometry and preserve the recorded results for their steering profiles.

## Verification

- Kinetra Debug build for weighted navigation, navigation, behavior, avoidance and translation sweeps. All 163 selected CTest cases passed.
- The external consumer built and ran with `CONSUMER_GRAPHICS=ON`.
- Lab pressure and golden baselines passed before the change. The same fixtures also passed through the shared API.
- Pull request CI covers the Full level.

## Review

The extraction preserves closest-approach ordering, strict overlap in the guard, the formation sweep's contact semantics and the final sample when the horizon is off cadence. Candidate acceptance and event ordering stay with i4sim. The existing solver composition and factory tuning were checked at their call sites.

## Pull request

Title: `refactor(navigation): share predictive passing geometry`

**Summary.** Move i4sim's tested conflict timing, lane candidate generation and per-neighbor guard into Kinetra's headless simulation library.

**Changes.** Expose passing geometry and axis-aligned sweep helpers to consumers. Require explicit priority and tangent steering tuning. The companion Lab change carries forward the existing i4sim values and preserves traffic policy and event handling.

**Design.** [Shared predictive passing](2026-11-26-shared-passing.md) and [ADR 0016](../decisions/0016-predictive-passing.md).

**Verification.** Debug CTest passed for the 163 affected engine cases. The external consumer built and ran. The companion Lab branch exercises the pressure and golden fixtures.

**Review notes.** Check the candidate order, the distinction between touching and overlap in the two sweeps, and the steering call sites that previously relied on defaults.

**Dependencies and merge order.** Kinetra first, with a merge commit. The companion Lab change links this pull request and records its Kinetra commit.

**Follow-up.** i4sim stall measurements inform the remaining shared policy decisions and adoption in the other labs.

## Open points

- Decide ownership of MAKE WAY and temporary motion fallback from the i4sim stall detection and remedies work. Demo and Dotio adoption follows the Lab roadmap item.
