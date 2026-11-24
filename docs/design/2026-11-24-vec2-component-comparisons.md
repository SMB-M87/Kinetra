# Vec2 equality in place of component comparisons

Status: accepted · Branch: `refactor/vec2-component-comparisons` · Related: [Vec2 equality](https://github.com/SMB-M87/Kinetra/pull/52)

Kinetra can use its existing vector equality in the simulation, launcher and tests wherever both coordinates are compared exactly.

## Table of contents

- [Problem](#problem)
- [Goals and non-goals](#goals-and-non-goals)
- [Decisions](#decisions)
- [Design](#design)
- [Affected areas](#affected-areas)
- [Test plan](#test-plan)
- [Review](#review)
- [Verification](#verification)
- [Pull request](#pull-request)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem

`same(Vec2, Vec2)` in `src/simulation/physics.cpp` duplicates the equality contract of `Vec2`. The launcher, avoidance and several tests also spell out exact comparisons of both coordinates.

## Goals and non-goals

Use the existing operators for exact comparisons of complete vectors. Preserve simulation results, pointer handling, per-axis predicates and tolerance-based checks. The companion application cleanup is tracked in the [Lab roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md#later).

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Equality | Use `Vec2::operator==` and the corresponding `!=` | Both use the same float comparisons as the existing predicates |
| Polygon geometry | Compare the vertex vectors directly | `std::vector` checks size and compares corresponding vertices with `Vec2::operator==` |

## Design

Compare fixed-body positions, box extents and polygon vertices directly. Compare avoidance extents and the calculated offset against zero vectors. Detect launcher pointer motion with vector inequality while preserving the settling guard. Express exact test expectations as vector comparisons.

## Affected areas

Simulation (`physics.cpp`, `avoidance.cpp`), launcher input handling and the simulation, weighted-navigation and runtime tests.

## Test plan

Run the existing simulation suites for fixed-body cache reuse and invalidation, collision and avoidance. Run the weighted-navigation reference comparisons and the launcher suite, including the resting-pointer regression. Run the runtime graphics suite for viewport resizing and coordinate picking. The existing vector equality case covers differing axes, signed zero and NaN. Pull request CI provides Full verification.

## Review

Pending.

## Verification

Pending.

## Pull request

Pending.
