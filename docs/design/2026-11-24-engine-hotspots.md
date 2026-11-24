# Engine hotspots

Status: draft · Branch: `perf/world-fixed-index` (step 1 of a stack) · Related: [roadmap, *Engine performance*](../roadmap.md#now), Kinetra-Lab's [performance baseline](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/performance.md#hotspots) and its roadmap item *Engine hotspots from the performance baseline*

Kinetra-Lab's performance baseline of 2026-11-23 found four hotspots in engine code: `World::step`'s broad phase, `SpatialGrid::query`, `priority_steer`'s prediction loop and `bounds` of a box. This change takes them one per pull request, in that order, without changing a single simulated value: every golden line, fingerprint and benchmark result stays as it is, and only the time goes down. Demo falls behind at time scale 4 because of the first one.

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

The numbers are Lab's baseline ([performance](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/performance.md)), measured on Lab `bae378f` with Kinetra `bc17b4f`:

1. **`World::step` rebuilds its grid from every body, at least 14 times per tick** ([physics.cpp](../../src/simulation/physics.cpp), `World::step`). Each swept event of a substep and each of its six correction passes calls `SpatialGrid::rebuild` with the box of every body. Demo's world has 489 bodies at 48 agents, 362 of them fixed terrain that never moves. The rebuilds are 64% of a Demo tick at 48 agents and 35% at 240; `rebuild` clears an `unordered_map` of vectors, which frees every bucket, and fills it again, so allocation is a third of the run at 48 agents.
2. **`SpatialGrid::query` hashes every cell, allocates and sorts on every call** ([spatial_grid.cpp](../../src/simulation/spatial_grid.cpp)): 23% of i4sim with predictive passing, where plan forming queries a square that grows with the horizon, 7% elsewhere in i4sim, 22% of Demo at 240 agents, and 9% of Demo inside `World::step`.
3. **`priority_steer` tests (horizon + 1)² pairs of predicted positions for every neighbor in reach** ([priority_steering.cpp](../../src/simulation/priority_steering.cpp)): 1,089 pairs at i4sim's horizon of 32, 22% of the motion benchmark, 15% with passing.
4. **`bounds` of a box calls `projected_radius` twice, each with its own `sin` and `cos`**: 7% of i4sim in `sincosf`. Most of those calls come from i4sim's contact resolution, which Lab's hotspot 2 changes on its own branch.

## Goals and non-goals

Goals:

- One Kinetra pull request per hotspot, stacked in the order above, each with a Lab pull request that points at it, measures before and after with the baseline's workloads and updates Lab's performance page.
- Behavior to the bit: Demo's crowd characterization lines (a 64-bit fingerprint of every moving body), i4sim's golden and pressure tests, i4sim's motion benchmark CSV apart from `elapsed_ms` (with and without `--predictive-passing`), Dotio's fingerprints and Kinetra's own tests stay identical. A golden line that moves means the step is wrong; none is updated.
- No public API change that a caller has to follow. A new overload is allowed; a changed meaning is not.

Non-goals:

- Lab's hotspot 2 (i4sim's `MotionStep::resolve_contacts`) and hotspot 6 (Dotio's shelter search), which are Lab code.
- Faster narrow phases (`collide`, `sweep_translation`) or a different broad-phase structure for `World`; the baseline does not point at them.
- Multithreading.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| How `World::step` learns that a fixed body changed | It compares the fixed bodies at the start of each step with copies of the ones the fixed index was built from (index, motion and every field `bounds` reads), and rebuilds the index only on a difference | `World::body(id)` returns a mutable `Body&`, and callers change bodies between steps (Demo assigns the player's body every tick). A comparison needs no API change and costs one pass over the fixed bodies per step, against 14 rebuilds. An explicit "terrain changed" call would be a contract change that every caller could forget |
| Order of the candidates | A query of both indexes is merged into the sorted, unique list one grid over all bodies gave | The earliest swept event breaks ties by pair order (strict `<`), and the correction passes resolve in order; any other order changes the simulation |
| Where the fixed index is rebuilt | At the first pass of a step, where the single grid used to be rebuilt | A bad box throws `std::invalid_argument` at the same point of the step, after the first substep's forces, as before. When a fixed and a moving box are both bad, the fixed one is checked first, so the message can name the other fault; the type and the point stay, and no test or caller reads the message |
| Which fields mark a fixed body as changed | Per shape, the fields `bounds` reads: a circle's position and radius; a box's position, half extents and angle; a polygon's position, angle and vertices | Turning a fixed circle, or any field `bounds` ignores, keeps the index |
| Bucket storage | `SpatialGrid::rebuild` keeps the vectors of the cells in use and drops a cell that stayed empty for a whole rebuild | Rebuilding over boxes that barely move then allocates almost nothing; the map stays bounded by the cells of the last two rebuilds |
| Stack | Four Kinetra pull requests, `perf/world-fixed-index`, then the query, the prediction loop and `bounds` on branches stacked on it; one Lab pull request per step on the Lab stack | One question per pull request, measured on its own |

## Design

### Step 1: `World::step` indexes fixed bodies once

`World` keeps two grids. `fixed_grid_` holds the boxes of the fixed bodies and is rebuilt only when one of them changes; `grid_` holds the boxes of the moving bodies and is rebuilt in every pass, as before. A fixed body has no velocity (`velocity()` in `step` returns zero for it), so its swept box is its plain box, and one index of fixed boxes is valid for every sweep and correction pass of a step. `resolve` changes a fixed body by adding a zero (inverse mass and inertia are zero), so its box stays the same during the step.

At the start of `step`, `fixed_index_current()` walks the bodies and compares each fixed body with the copy the index was built from: same index, and the same `shape`, `position`, `half_extents`, `radius`, `angle` and `vertices`, the fields `bounds` reads. Any difference, a body that became fixed or stopped being fixed, or a different number of fixed bodies, marks the index stale; the first pass of the step rebuilds it. A comparison with `==` treats `-0` and `+0` as equal, which changes no overlap test.

A query merges the two grids' results: both ascend in body index, so the merge gives the sorted, unique list a single grid gave, and the loops over `j` see the same candidates in the same order. In a sweep pass a fixed body only meets moving ones (fixed pairs are skipped before they count), so it queries only the moving grid; that changes neither the pairs tested nor `candidate_pairs()`. The correction passes keep using the boxes taken at the start of the pass.

`SpatialGrid::rebuild` keeps the bucket vectors of the cells in use instead of freeing them; a cell that was already empty after the previous rebuild is erased. Query results do not depend on empty buckets.

### Step 2: `SpatialGrid::query` without hashing, allocation and sort

`rebuild` validates every box first, as before, and keeps each box's cell range. When the cells all boxes cover form a compact area, at most `4 * entries + 1024` cells and at most 2^22 (32 MB of offsets), the grid stores them densely: `starts_` holds, for each cell of that area row by row, where its boxes begin in `ids_`, which is filled in index order, so each cell lists its boxes ascending. Otherwise the hash map of step 1 stays. A rebuild that fails on an allocation leaves an empty index instead of mixed storage.

`query` clips the query's cell range to the covered area and visits its cells. A box that spans several of them is reported only from the first cell of its overlap with the query, its lowest `x` and `y` there, so the result holds no duplicates and needs no `unique`; overlapping boxes always share that cell, because floor is monotonic. One cell lists its boxes ascending, so a one-cell query skips the sort. A new overload, `query(Aabb, std::vector<std::size_t> &)`, writes into a vector the caller reuses; the old one calls it. The result is the same sorted, unique set; the limits (100,000 cells per box, cell coordinates within ±10^9) and the exceptions stay, and a rejected query leaves the caller's vector unchanged.

`World::step` queries both of its grids through member vectors. Lab's callers keep the returning overload: in the profile after this step allocation is 4% of Demo and 1% of i4sim, and a query that tested every box when it would visit more entries than there are boxes made no measurable difference on Demo, so neither is part of this step.

### Step 3: `priority_steer` finds the first overlapping pair per step

To be designed on its branch, from this direction: for each step `i` of the mover, the predicted gap to the neighbor is linear in the neighbor's step `j`, so the steps `j` that can overlap form an interval. Only the steps of that interval, widened by a bound on the rounding error, are tested with the existing expression, in ascending order, so the first pair `(i, j)` and its `diff` stay the same.

### Step 4: `bounds` of a box with one rotation

Measured again after Lab's hotspot 2 has merged: either `bounds` computes `sin` and `cos` once per call with values identical to the bit, or Lab's performance page and both roadmaps record why it is no longer worth a pull request.

### Steps and branches

| Step | Kinetra branch | Kinetra pull request | Lab branch | Lab pull request |
| --- | --- | --- | --- | --- |
| 1. Fixed bodies indexed once | `perf/world-fixed-index` | | `ccr-perf-engine-hotspots` | |
| 2. Spatial query | `perf/spatial-query` | | `ccr-perf-engine-hotspots-hotspot3` | |
| 3. Prediction loop | `perf/priority-prediction` | | `ccr-perf-engine-hotspots-hotspot4` | |
| 4. Box bounds | `perf/box-bounds` | | `ccr-perf-engine-hotspots-hotspot5` | |

## Affected areas

- Step 1: `World` in [physics.h](../../include/kinetra/simulation/physics.h) (private members only) and [physics.cpp](../../src/simulation/physics.cpp); `SpatialGrid::rebuild` in [spatial_grid.cpp](../../src/simulation/spatial_grid.cpp). No public signature or documented behavior changes.
- Step 2: `SpatialGrid` in [spatial_grid.h](../../include/kinetra/simulation/spatial_grid.h) (a new `query` overload; private members) and [spatial_grid.cpp](../../src/simulation/spatial_grid.cpp); `World::candidates` in physics.cpp. Behavior and limits unchanged.
- Lab: [performance](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/performance.md) per step, the roadmap items in both repositories.

## Test plan

| Behavior | Test | Level |
| --- | --- | --- |
| Step 1: a fixed body the caller moves, reshapes, turns into a moving body, or adds between steps is seen by the next step | New cases in `simulation_tests.cpp`: a world stepped once, then changed through `body(id)` or `add`, collides as a fresh world with the same bodies does, with the same `candidate_pairs()` | Targeted |
| Step 1: the candidate pairs and their order are unchanged | New characterization case in `simulation_tests.cpp`: a 51-body world of fixed boxes, polygons and circles interleaved with moving ones in a container added last, whose fingerprint over 360 steps (every position, velocity, angle and `candidate_pairs()`) was recorded on Kinetra `73adc66`; putting the fixed candidates first in the merge changes it. Existing `moving_contact_tests.cpp` and `simulation_tests.cpp` (`candidate_pairs()`), Lab's Demo crowd characterization lines, i4sim golden and pressure tests, Dotio fingerprints | Full |
| Step 1: a rebuild keeps the grid's results | Existing spatial grid tests, plus a case that rebuilds with boxes that move away and back and queries after each rebuild | Targeted |
| Step 2: queries give the same sorted, unique indices in both storages, also through the caller's buffer | New cases in `simulation_tests.cpp`: a compact and a scattered set (one far box) against an exhaustive search, with large and empty areas; one grid switched between both storages and a rejected rebuild after a dense one; a rejected query leaves the buffer unchanged. The existing grid cases | Targeted |
| Every step: no simulated value changes | Lab's motion benchmark CSV, with and without `--predictive-passing`, identical to the baseline apart from `elapsed_ms` | Full (Lab) |

## Review

### Step 1

`/code-review` at medium on `main...perf/world-fixed-index` found no change to the bit-identical behavior and seven points:

| Finding | Resolution |
| --- | --- |
| No test compares the merged order with the single grid's; a fresh world uses the same merge | Fixed: the mixed-world fingerprint, recorded on `main`, fails when the merge puts fixed candidates first |
| Two bad boxes can give the other message | Recorded in *Decisions*: the type and the point of the throw are kept |
| `clear()` keeps the fixed index and its copies | Fixed: `clear()` drops them |
| Two grids allocate two vectors per query | Kept for step 2, whose buffer overload removes them; a world without fixed bodies skips the fixed query now |
| Every field is compared, whatever the shape | Fixed: the fields `bounds` reads for the shape |
| The member comment misstates `boxes_` | Fixed |
| The kept buckets cost a walk over the cells of two rebuilds | Kept: one walk over the cells against freeing and allocating every bucket; the measurement shows the gain. Step 2 replaces the hash map for compact areas |

### Step 2

`/code-review` at medium on `perf/world-fixed-index...perf/spatial-query` found no change to the query results and five points:

| Finding | Resolution |
| --- | --- |
| A rebuild that runs out of memory after setting the extent could leave storage that belongs to other boxes, and a later query reads out of bounds | Fixed: an allocation failure leaves an empty index |
| A dense rebuild freed the hashed buckets | Fixed: dense rebuilds leave the map alone; the next hashed rebuild clears it as in step 1 |
| The dense fill copied the offsets into a scratch vector | Fixed: the offsets serve as fill positions and are shifted back |
| No test switches one grid between both storages | Fixed: compact, scattered, compact, then a rejected rebuild that keeps the dense index |
| `<utility>` missing for `std::move` | Fixed |

## Verification

### Step 1

- Kinetra: `cmake --preset dev && cmake --build --preset dev && ctest --preset dev`: 272 of 272 passed. `clang-format --dry-run --Werror` on the changed files, `python3 tools/check_docs.py` and `python3 tools/check_copies.py --term 'rebuilds its grid' --term 'broad phase'` (every hit still true) pass.
- Lab at the pointer: `ctest --preset dev` without the graphics cases, 318 of 318 passed, golden and pressure tests included.
- Bit identity on the release build: i4sim's motion benchmark CSV, with and without `--predictive-passing`, identical to the baseline's apart from `elapsed_ms` in all three runs; Dotio's output, fingerprints included, identical; Demo's characterization lines pass.
- Time, release preset, median of three runs on one cloud machine (four cores of an Intel Xeon at 2.1 GHz, about 1.25 times slower than the baseline's on Demo): Demo's soak at 48 agents 12.1 to 2.7 s (steering) and 12.0 to 2.5 s (route), at 240 agents 28.0 to 18.1 s, 28.9 to 18.5 s and 26.0 to 16.3 s; i4sim and Dotio within run-to-run noise. Lab's performance page has the row and the new Demo profile.

### Step 2

- Kinetra: `ctest --preset dev`, 276 of 276 passed; `clang-format --dry-run --Werror` on the changed files.
- Bit identity on the release build, on Lab `main` with hotspot 2: i4sim's CSV with and without `--predictive-passing` and Dotio's output identical in all three runs; Demo's characterization lines pass.
- Time, median of three runs interleaved with step 1 on the same machine: i4sim 19.8 to 15.8 s, with passing 34.3 to 23.1 s; Demo's soak 2.9 to 1.6 s at 48 agents, 18.8 to 13.3 s at 240; Dotio unchanged. A query that tested every box when it would visit more entries than there are boxes measured the same on Demo (13.2 against 13.4 s over three interleaved runs) and was left out.

## Pull request

Filled in per step.

## Open points

None yet.
