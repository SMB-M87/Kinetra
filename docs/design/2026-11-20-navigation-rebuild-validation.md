# Navigation rebuild validates before it changes state

Status: draft · Branch: `fix/navigation-rebuild` · Related: [navigation contracts](../engine/navigation.md#contracts), [simulation guide, open points](../../src/simulation/README.md#open-points)

`Navigation::rebuild` stored the new area and obstacle copy before it validated the bodies. An invalid body therefore left an instance whose area and size no longer matched its caches, and `path()` on it read the old caches with the new size. This change validates every body first, so a rejected rebuild leaves the previous snapshot intact.

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
- [Open points](#open-points)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem

In [navigation.cpp](../../src/simulation/navigation.cpp), `rebuild` assigned `area`, `width`, `height` and `geometry`, and only then called `validate(body)` per obstacle. When a body was invalid (a NaN position, a non-positive size), the throw left:

- `area`, `width` and `height` describing the new grid;
- `geometry` holding the new, invalid bodies;
- `index`, `blocked`, `links` and `region` still sized and filled for the old grid.

`path()` indexes `blocked`, `links` and `region` with `y * width + x` for the new `width` and `height`. With a larger new area that reads past the end of the old vectors, which is undefined behavior. With a smaller one it silently plans in the wrong grid. `clear()` looked up old index entries in the new `geometry`. The header documented the state ("half updated: rebuild again before planning"), and the [simulation guide](../../src/simulation/README.md#open-points) listed it as an open point.

The reproduction test (see [Test plan](#test-plan)) failed before the fix: after a rejected rebuild with a smaller area, the path between two points of the old area was empty instead of the three waypoints it had before.

## Goals and non-goals

- Goal: a rebuild that throws `std::invalid_argument` changes nothing; `path()` and `clear()` keep answering from the previous snapshot.
- Goal: the header, the contract table and the simulation guide state that guarantee.
- Non-goal: a guarantee on allocation failure. `rebuild` still allocates after validation; the [contract](../engine/navigation.md#contracts) keeps saying it is no transaction on `std::bad_alloc`.
- Non-goal: faster validation or a change to what counts as an invalid body.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| How to keep the old snapshot | Validate the bounds and every body before the first member is assigned | Everything after validation only allocates and computes from valid input, so invalid input can no longer throw halfway. Building every cache into locals and swapping them in would also cover `std::bad_alloc`, but `clear()` and `point()` read the members, so that needs a larger restructuring for a failure the contract already excludes |

## Design

`rebuild` now runs in two phases:

1. **Check.** The bounds and size checks stay first. Then every obstacle goes through `kinetra::validate`, and its bounding box is collected into a local `boxes` vector.
2. **Commit.** Only then are `area`, `width`, `height` and `geometry` assigned and the index and caches rebuilt.

No behavior changes for valid input: the same boxes reach `index.rebuild`, in the same order.

## Affected areas

- `src/simulation/navigation.cpp`: the order inside `Navigation::rebuild`.
- `include/kinetra/simulation/navigation.h`: the `rebuild` summary (public header, comment only).
- `docs/engine/navigation.md`: the `rebuild` row of the contract table.
- `src/simulation/README.md`: the validation invariant and the open point.
- `tests/navigation_tests.cpp`: one new case.

## Test plan

| Behavior | Test | Level |
| --- | --- | --- |
| A rebuild with an invalid body throws and leaves the previous snapshot: same path, same blocking | `kinetra_navigation:a rebuild with an invalid body throws and keeps the previous snapshot` (fails before the fix, with an empty path) | Targeted |
| Valid rebuilds plan as before | The existing navigation, Lab Demo and i4sim suites | Full |

The public header changes, so the branch ends at the Full level.

## Review

To be filled in after the review.

## Verification

To be filled in after verification.

## Pull request

To be filled in at the end.

## Open points

- None.
