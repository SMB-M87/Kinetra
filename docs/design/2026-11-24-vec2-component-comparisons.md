# Vec2 equality in place of component comparisons

Status: implemented · Branch: `refactor/vec2-component-comparisons` · Related: [Vec2 equality](https://github.com/SMB-M87/Kinetra/pull/52)

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

The branch diff preserves shape-specific cache checks, including polygon vertex count and order. The avoidance test still compares the calculated offset against zero, and pointer movement still requires the settling guard. Tests assert exact equality on the same coordinates. The axis-alignment predicate in `priority_steering.cpp` tests for a difference on both axes and retains its component comparisons.

## Verification

- `cmake --preset dev` and `cmake --build --preset dev --parallel 3 --target kinetra_tests kinetra_polygon_tests kinetra_sweep_tests kinetra_translation_sweep_tests kinetra_moving_contact_tests kinetra_contact_response_tests kinetra_avoidance_tests kinetra_weighted_navigation_tests kinetra_launcher_tests` passed.
- `ctest --preset dev --parallel 3 -R '^kinetra_(behavior|polygons|sweeps|translation_sweeps|moving_contacts|contact_response|avoidance|weighted_navigation|launcher)'` passed all 202 tests.
- `cmake --preset graphics`, `cmake --build --preset graphics --parallel 2 --target kinetra_engine_runtime_tests` and `ctest --preset graphics -R '^kinetra_engine_graphics:'` passed all 20 runtime tests with SDL offscreen and software Mesa.
- In Kinetra-Lab, `cmake --preset graphics`, `cmake --build --preset graphics --parallel 3 --target kinetra kinetra_menu_tests` and `ctest --preset graphics -R '^(kinetra_menu:|kinetra_smoke:launcher$)'` passed all 10 tests, including the offscreen launcher smoke test, against the engine refactor.
- `clang-format --dry-run --Werror` with version 23.1.5 passed for the six changed C++ files. `python3 tools/check_docs.py`, `python3 tools/check_copies.py` and `git diff --check` also passed.
- Pull request CI provides Full verification.

## Pull request

**Title** `refactor(engine): use Vec2 equality for exact comparisons`

**Summary**

Use Kinetra's existing exact `Vec2` equality throughout the engine where the code compares both coordinates. This completes the Kinetra roadmap item with the same name.

**Changes**

- Compare positions, box extents and polygon vertex vectors directly in the fixed-body cache.
- Use vector comparisons for zero vectors in avoidance and for pointer motion in the launcher.
- Use vector comparisons for exact expectations in the simulation, weighted-navigation and runtime tests.

**Design**

[Vec2 equality in place of component comparisons](https://github.com/SMB-M87/Kinetra/blob/refactor/vec2-component-comparisons/docs/design/2026-11-24-vec2-component-comparisons.md)

**Verification**

The targeted `dev` build and 202 tests passed. The `graphics` runtime target and 20 tests passed on SDL offscreen with software Mesa. The Kinetra-Lab launcher build and 10 tests, including its offscreen smoke test, passed against the refactor. Formatting, documentation links and the copies check passed. Full verification runs in pull request CI.

**Review notes**

Polygon vertex comparison still checks size, order and both coordinates. The avoidance comparison uses the same calculated offset. Per-axis predicates and tolerance-based checks keep their existing semantics.

**Dependencies and merge order**

Kinetra #52 is already on `main`. The Lab pointer workflow follows the Kinetra merge.

**Follow-up**

The companion i4sim, Demo and capture changes are tracked in the [Kinetra-Lab roadmap](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/roadmap.md#later).
