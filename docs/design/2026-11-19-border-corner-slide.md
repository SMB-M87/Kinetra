# Border corner slide

Status: implemented · Branch: `fix/border-corner-slide` · Related: [Lab design](https://github.com/SMB-M87/Kinetra-Lab/blob/feat/i4sim-h-crossing-motion/docs/design/2026-11-19-i4sim-h-crossing-motion.md), [navigation](../engine/navigation.md#weighted-grids-and-priority-steering)

`constrain_borders` stops a box that slides flush along one wall when its corner meets the end of another wall at a convex corner. The box then stands still for good. Kinetra-Lab's i4sim needs movers that steer straight at their goal without a route ("path planning off"), and in its H-shaped hall every such mover stuck on a convex corner of the middle bar. This change picks the constrained face beside a segment's end the way a swept-box test does.

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

- For each border segment, `constrain_borders` ([priority_steering.cpp](../../src/simulation/priority_steering.cpp)) grows the segment by the box's half extents and measures the gap to that box per axis. It constrains the axis with the larger gap: `x_axis = gap.x >= gap.y`.
- A box sliding flush along a floor has `gap.y == 0` to every wall that starts at the floor's level. Approaching the end of the floor where a wall drops away (a step, or the corner of a bar in a hall), `gap.x > gap.y`, so the drop's x face is constrained: the box brakes as if that wall stood in its way, and stops with both gaps at zero. The floor constrains y and the drop constrains x, so the box can never move again, although moving on along the floor would touch nothing.
- Measured in Lab: in the i4sim H-shaped hall with route planning switched off, all four movers stopped for good at the four inner corners of the middle bar, at (1143.5, 663.5) and its mirror images, within 1000 ticks.

## Goals and non-goals

- Goal: a box that slides flush along one wall passes the convex corner where another wall begins.
- Goal: no change inside a segment's span (one gap negative) or in an overlap (both gaps negative); braking toward a wall ahead stays as it is.
- Goal: no public API change.
- Non-goal: rotated bodies, circles, or a full continuous collision test; `constrain_borders` stays a one-step velocity constraint per segment.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Where to fix | In Kinetra, not by a workaround in Lab | User's choice. The trap is in the reusable border constraint, and any consumer sliding along walls can hit it. A Lab workaround (route recovery after a stall) leaves movers standing at every corner first |
| Rule beside a corner | The face the box would reach last at its current velocity, as in a swept-box test; a face it does not approach counts as never reached; on a tie the larger gap, as before | A box only hits a face once its projection overlaps the segment on the other axis as well. A flush slide never makes that overlap, so the face it slides along is the one that matters. The tie rule keeps the old choice for a box at rest or on an exact diagonal |
| Velocity used | The current velocity, not the requested acceleration | The requested acceleration usually points into the floor as well (toward a goal below the step); judged by it, the y projection would start overlapping at once and the trap would remain. The floor's own constraint removes that component |

## Design

```mermaid
flowchart TD
    G["gaps of the box to one grown segment"] --> B{"both gaps >= 0?"}
    B -->|no: inside the span or overlapping| L["constrain the axis with the larger gap (unchanged)"]
    B -->|yes: beside a corner| T["time to reach each face at the current velocity; never when not approaching"]
    T --> C{"times differ?"}
    C -->|yes| Last["constrain the face reached last"]
    C -->|no| L
```

The takeaway: only the corner region changes. A box sliding flush along the floor never reaches the drop's x face (its y projection never overlaps the drop), so the drop constrains y, where the box does not move, and the slide continues.

## Affected areas

- `src/simulation/priority_steering.cpp`: the axis choice in `constrain_borders`.
- `include/kinetra/simulation/steering.h`: the contract of `constrain_borders` names the corner rule.
- `docs/engine/navigation.md`, `tests/README.md`.
- `tests/weighted_navigation_tests.cpp`: one new case.
- Consumers: only Kinetra-Lab's i4sim calls `priority_steer` and `constrain_borders`.

## Test plan

| Behavior | Test | Level |
| --- | --- | --- |
| One unit before the corner, the slide keeps its full tangential acceleration and does not push into the floor | `kinetra_weighted_navigation`: a body sliding along a wall passes the convex corner where the wall ends | Targeted |
| Driven toward a goal below the step, the box passes the corner without cutting it and arrives | Same case, 200 integration steps | Targeted |
| Braking toward walls, corner approaches and parking convergence stay as they are | Existing `kinetra_weighted_navigation` cases | Targeted |
| i4sim behavior, including golden and pressure summaries, stays the same | Lab's full test suite against this commit | Full (Lab) |

The new case failed before the fix: the near-corner acceleration was -0.6 instead of 0.2, and the box stopped 57 units from its goal. A public header changes (a comment), so the branch ends at the Full level.

## Review

Reviewed as its own diff, next to the Lab change that needs it: only the corner region changes; inside a segment's span and in an overlap the old rule stays; a body at rest or approaching exactly diagonally keeps the old choice (larger gap); the result stays one bounded step per segment, so the acceleration bound and the existing braking cases are unchanged. No findings.

## Verification

Full level (a public header comment changed).

| Command | Result |
| --- | --- |
| `cmake --workflow --preset ci` | Passed: 223 of 223 tests, no compiler warnings |
| `cmake --workflow --preset headless` | Passed: 201 of 201 tests |
| External consumer (`tests/consumer`, `CONSUMER_GRAPHICS=ON`) | Builds and runs |
| `python3 tools/check_docs.py` | All links resolve |
| `clang-format --dry-run --Werror` on the changed sources | Clean |
| New case without the fix | Fails: near-corner acceleration -0.6 instead of 0.2; the box stops 57 units from its goal |
| Kinetra-Lab against this commit (`ci`, `headless`, `graphics`) | See the [Lab design](https://github.com/SMB-M87/Kinetra-Lab/blob/feat/i4sim-h-crossing-motion/docs/design/2026-11-19-i4sim-h-crossing-motion.md#verification); its golden and pressure summaries are unchanged |

## Pull request

Title: `fix(steering): let a box sliding along a wall pass a convex corner`

**Summary.** `constrain_borders` stopped a box that slid flush along one wall when its corner met the end of a wall dropping away from it: it braked as if that wall stood in its way, and then stood still for good. Beside a segment's end it now constrains the face the box would reach last at its current velocity, as a swept-box test does. Kinetra-Lab needs this for i4sim movers that steer straight at their goal (path planning off).

**Changes.**
- Steering: the axis choice in the corner region of `constrain_borders`; inside a segment's span and in overlaps nothing changes.
- Contract: `steering.h` and the navigation page name the corner rule.
- Tests: a box sliding over a step passes its convex corner, both one unit before it and over 200 integration steps.

**Design.** [docs/design/2026-11-19-border-corner-slide.md](2026-11-19-border-corner-slide.md)

**Verification.** `cmake --workflow --preset ci` (223/223), `headless` (201/201), the external consumer, docs check, clang-format. The new case fails without the fix. Kinetra-Lab's full suite, including its golden and pressure summaries, passes unchanged against this commit.

**Review notes.** The velocity used is the current one, not the requested acceleration; the design document explains why the latter would keep the trap.

**Dependencies and merge order.** None. Kinetra-Lab's `feat/i4sim-h-crossing-motion` points at this branch: merge this pull request first, with a merge commit, then move the Lab pointer to the merge commit.

**Follow-up.** None.

## Open points

- None known.
