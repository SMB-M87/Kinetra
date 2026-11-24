# Raster route target

Status: implemented · Branch: `feat/raster-route-target` · Related: [navigation](../engine/navigation.md#weighted-grids-and-priority-steering), [Dotio restructuring](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-21-dotio-restructuring.md#open-points)

Let an agent following a raster route steer past intermediate waypoints once it has swept clearance to the next target.

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

[`route_target`](../../include/kinetra/simulation/navigation.h) advances past visible waypoints and provides corner lookahead for routes that use body geometry. [`WeightedGrid`](../../include/kinetra/simulation/weighted_grid.h) has a swept clearance query, but its callers would have to implement that following logic themselves. Dotio currently advances by distance to a waypoint.

## Goals and non-goals

Provide the same route-following operation for a raster, with caller-supplied lookahead and support for a swept rectangular footprint. Preserve the exact final waypoint and the existing geometry overload. Dotio adoption belongs to [Smoother pursuit in Dotio](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md#later).

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Lookahead | A nonnegative distance in the raster's world units, supplied in `GridRouteOptions` | Consumers use different scales, including legacy pixels in Dotio. Zero allows waypoint skipping alone. |
| Shortcut criterion | The swept geometry in `WeightedGrid::clear` | The same occupancy and footprint apply to planning and following. Callers that need every weight or `search` admission decision preserved can follow the original waypoints. |
| Route ownership | The caller supplies the remaining waypoints and owns arrival and replanning | Matches the geometry overload and lets applications act when their raster changes. |

## Design

The overload `route_target(position, route, grid, options)` works on the same remaining waypoint vector as the geometry overload. Starting at the leading waypoint, it tests direct clearance to the next one. Each success advances the leading waypoint. At the first obstruction, it tries a point along the next leg, with a distance budget reduced by the distance to the corner. If that point is visible, it is the target for this update. Otherwise the leading waypoint is the target. The caller supplies a valid route with a reachable leading waypoint.

Both overloads share the advancement and lookahead logic. Check candidates before erasing the consumed prefix once. An empty route returns the current position and a route with one remaining waypoint returns that exact point. The raster overload validates position, footprint and lookahead before it starts.

## Affected areas

- Public overload in `weighted_grid.h` and shared following logic in `navigation.cpp`.
- Raster navigation tests and the navigation contract in the engine guide.

## Test plan

| Behavior | Check |
| --- | --- |
| Visible waypoints are consumed, the exact goal is retained, and an empty route returns the current position | Weighted navigation tests |
| A blocked corner bounds the target, with lookahead only when the swept segment is clear | Weighted navigation tests with a blocked center cell |
| The full footprint participates in shortcut decisions | Weighted navigation tests comparing a point with a footprint on the same route |
| Invalid options throw before changing the route | Weighted navigation tests |
| The geometry overload retains its behavior | Navigation regression tests |

Local verification uses both navigation suites in `dev`. The public overload also requires the pull request's Full CI checks.

## Review

Author review covered the old overload's two-unit budget, blocked shortcuts, footprint clearance, input validation before mutation, and retention of the exact goal. The API and its tests are in the headless simulation layer.

## Verification

- `cmake --preset dev` and `cmake --build --preset dev --target kinetra_navigation_tests kinetra_weighted_navigation_tests` passed.
- `ctest --preset dev -R '^kinetra_(navigation|weighted_navigation)'` passed all 53 cases.
- `clang-format --dry-run --Werror` passed on the four changed C++ files with clang-format 23.1.5.
- `python3 tools/check_docs.py` and `git diff --check` passed. `python3 tools/check_copies.py` flagged references to `WeightedGrid` and `route_target` after the roadmap entry was removed. The references still describe the live APIs.
- Full CI and the separate agent review are tracked on the pull request.

## Pull request

`feat(navigation): add raster route targets`

### Summary

Agents following a `WeightedGrid` route can skip visible intermediate waypoints and look past clear corners using the same swept clearance as the grid planner.

### Changes

- Add a raster `route_target` overload with lookahead in grid world units and an optional rectangular footprint.
- Share waypoint advancement with the body navigation overload and retain its existing target selection.
- Test exact goals, blocked corners, footprint-dependent visibility, and invalid inputs.

### Design

`docs/design/2026-11-24-raster-route-target.md`

### Verification

Both `dev` navigation targets built and all 53 selected CTest cases passed. Formatting, documentation links, and whitespace checks passed. The copies check found valid references to the API names from the retired roadmap entry. Full CI runs on this pull request.

### Review notes

Raster shortcuts use geometry. Route weights and `search` admission rules remain planner inputs. The caller owns replanning when its raster changes.

### Dependencies and merge order

This pull request targets Kinetra `main`. The Lab pointer follows the Kinetra merge through its existing workflow.

### Follow-up

Lab tracks using the overload in Dotio under *Smoother pursuit in Dotio*.

## Open points

- Dotio can use the overload in its separate pursuit improvement.
