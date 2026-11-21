# Weighted grid search for a cell with a property

Status: implemented · Branch: `feat/weighted-grid-search` · Related: [navigation guide](../engine/navigation.md#weighted-grids-and-priority-steering), [ADR 0011](../decisions/0011-weighted-navigation-and-priority-steering.md), Kinetra-Lab design `docs/design/2026-11-21-dotio-restructuring.md` (roadmap phase 6)

`WeightedGrid::path` plans to a known goal. Kinetra-Lab's Dotio also searches for the nearest cell that has a property: a shelter its enemies cannot be seen from. This change adds `WeightedGrid::search`, which returns the cheapest route to the first cell, in order of route cost, that a predicate accepts, through cells a second predicate admits. Dotio can then drop its own A* and shelter search.

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

Lab roadmap phase 6 replaces Dotio's own `Graph`, `Vertex`, `PriorityQueue` and `Stack` with Kinetra's planner. Dotio's raster of square cells maps directly onto `WeightedGrid` (same eight-neighbor moves, no corner cutting, per-cell `block`). One query has no counterpart: `Graph::HideStar`, the recovery state's shelter search, expands cells outward from the enemy on the side away from the player and takes the cell after a number of cells that are out of the player's sight. `WeightedGrid` has only `path(start, goal)`.

Searching for the nearest cell with a property (cover, a free spot, an exit) is a mechanism; which property counts is application policy.

## Goals and non-goals

**Goals:** a search with the moves, costs and corner rule of `path()`, a caller-supplied acceptance predicate called in a defined order, and a caller-supplied filter of enterable cells.

**Non-goals:** changing `path()` (its results stay bit-identical, which i4sim's golden test checks), a cost limit, multiple results.

## Decisions

Agent decisions, made while the user was not available; recorded for review in the Lab design document's open points.

| Decision | Choice | Why |
| --- | --- | --- |
| Shape | `search(start, accept, enter = {}, half_extents = {})` returning waypoints like `path()` | The same result type lets callers follow both kinds of route alike |
| Order | Dijkstra: each reachable cell offered once, the start's cell first, then in nondecreasing route cost | A defined order makes a counting predicate meaningful; without a goal there is no heuristic |
| Predicates | `std::function<bool(int cell)>` | Called once per settled cell or edge; the overhead is small next to the clearance test, and the header stays free of templates |
| Shared code | `path()` and `search()` share a private expansion template, defined and instantiated only in the `.cpp` | One expansion loop; `path()` keeps inlined lambdas, so its speed and results do not change |
| ADR | None | An additive function on an existing mechanism; no boundary changes |

## Design

- `expand(first, half_extents, point, heuristic, accept, enter)` holds the best-first loop that `path()` had: it settles cells in order of cost plus heuristic and returns the route to the first settled cell `accept` takes, ending at `point` of that cell.
- `path()` calls it with the exact start and goal as points, the Euclidean heuristic, "is the goal cell" as `accept` and no filter. Its endpoint checks and the same-cell case stay before the call.
- `search()` validates the half extents, rejects a nonfinite, blocked or outside start, offers the start cell to `accept` (route: that cell's center), and otherwise calls `expand` with a zero heuristic, `accept` for every other cell and `enter` as the filter. The route ends at the accepted cell's center.

## Affected areas

`include/kinetra/simulation/weighted_grid.h`, `src/simulation/weighted_grid.cpp`, `tests/weighted_navigation_tests.cpp`, `docs/engine/navigation.md`, this document and the design index.

## Test plan

| Behavior | Test (`kinetra_weighted_navigation`) |
| --- | --- |
| Nearest accepted cell by route cost, route through cell centers | `a search ends at the center of the accepted cell nearest by route cost` |
| Order and uniqueness of offers | `a search offers every reachable cell once in nondecreasing route cost, the start first` |
| Counting predicate | `a search counts accepted cells in route order, as a counting predicate expects` |
| Enter filter, unreachable result | `a search enters only admitted cells and fails when none leads to an accepted one` |
| Weights and corner rule | `a search follows weights and blocked corners like path` |
| Accepted start | `a search whose start cell is accepted goes to that cell's center` |
| Invalid input | `a search from a blocked or outside start is empty and negative extents throw` |
| `path()` unchanged | Existing weighted navigation cases, and Kinetra-Lab's i4sim golden test |

Verification level: Full (public header).

## Review

Filled in after review.

## Verification

Filled in after verification.

## Pull request

Filled in when the pull request is opened.

## Open points

None.
