# Core tests

These executables verify reusable engine behavior. Application tests belong to Kinetra-Lab.

## Table of contents

- [Test map](#test-map)
- [Execution](#execution)

[Parent guide](../README.md) · [Documentation index](../docs/README.md)

Directory guides: [consumer](consumer/README.md).

## Test map

| Source or directory | CTest name | Responsibility |
| --- | --- | --- |
| [traffic_tests.cpp](traffic_tests.cpp) | `kinetra_traffic:<case>` (doctest) | FIFO ordering, eligibility/ticket preservation, non-revoking grants, new arrivals, cancellation, occupied/contested zones, invalid snapshots and reset |
| [avoidance_tests.cpp](avoidance_tests.cpp) | `kinetra_avoidance:<case>` (doctest) | Acceleration/speed limits, infeasibility, deterministic IDs, time units, noncooperative motion, rectangular gap/cutoff/tangent checks, 2/4/8 rectangular encounters and 2/4/8/16 circular-agent arrival without overlap |
| [moving_contact_tests.cpp](moving_contact_tests.cpp) | `kinetra_moving_contacts:<case>` (doctest) | Relative queries, fast moving pairs, crossing/overtaking, chronological chains, spinning tangent-contact progress, narrow resting gaps, tiny-body contact tolerance and observable event-budget fallback |
| [translation_sweep_tests.cpp](translation_sweep_tests.cpp) | `kinetra_translation_sweeps:<case>` (doctest) | All convex translation pairs, timing, orientation, misses, thin walls, repeated impacts and free rotational integration |
| [sweep_tests.cpp](sweep_tests.cpp) | `kinetra_sweeps:<case>` (doctest) | Circle sweep timing, corners, containment, rotated geometry, thin-wall impacts, remaining travel and bounded repeated bounces |
| [polygon_tests.cpp](polygon_tests.cpp) | `kinetra_polygons:<case>` (doctest) | Convex shape validation, exact contacts, transformed geometry, inertia, navigation and falling-body settling |
| [simulation_tests.cpp](simulation_tests.cpp) | `kinetra_behavior:<case>` (doctest) | Geometry, physics, steering (including rectangular clearance and separation), spatial and collision grids, fixed clock, camera, invalid input and a 400-body stability run |
| [navigation_tests.cpp](navigation_tests.cpp) | `kinetra_navigation:<case>` (doctest) | Settings validation, swept clearance, exact and projected goals, and the math boundaries planning relies on |
| [weighted_navigation_tests.cpp](weighted_navigation_tests.cpp) | `kinetra_weighted_navigation:<case>` (doctest) | Weighted grid planning, footprint clearance and priority steering; see the detail below |
| [launcher_tests.cpp](launcher_tests.cpp) | `kinetra_launcher` | Window-free launcher behavior; requires runtime build |
| [runtime_tests.cpp](runtime_tests.cpp) | `kinetra_engine_graphics` | Runtime checks with a real OpenGL context |
| [consumer/](consumer/README.md) | Separate executable | Public headers and CMake source integration |

## Execution
```mermaid
flowchart LR
    Configure[Configure with BUILD_TESTING] --> Build[Build test executables]
    Build --> CTest[Run CTest]
    CTest --> Simulation[Simulation and navigation checks]
    CTest --> Launcher[Launcher checks when runtime enabled]
    CTest --> Graphics[Graphics checks when runtime and graphics tests enabled]
    Consumer[Separately configure consumer fixture] --> Public[Build and run public API consumer]
```

CTest coverage follows the configured options. The separate consumer checks source integration; graphics tests require an actual OpenGL context.


The root [CMakeLists.txt](../CMakeLists.txt) registers tests when `BUILD_TESTING` is enabled: doctest suites with `kinetra_add_doctest` (one CTest entry per case). Only the runtime-dependent launcher and graphics suites remain plain `main()` programs, registered with `kinetra_add_test`; `ctest -R '^<prefix>'` selects a whole doctest suite. The traffic suite is the doctest reference; how to write and run cases is described in [testing](../docs/testing.md#writing-a-doctest-suite). Graphics tests also require `KINETRA_BUILD_RUNTIME` and `KINETRA_TEST_GRAPHICS`. The consumer is configured separately.

See [development](../docs/development.md) for commands, dependencies and graphics backend requirements; see the [implementation map](../src/README.md) for tested modules.

`kinetra_weighted_navigation` covers weighted detours, blocked diagonal corners, invalid cost rollback, arrival braking, safety priority, acceleration limits and coincident bodies. It also covers swept rectangular grid clearance, merged wall segments, continuous bounded reactions at contact, tangential steering, close-wall convergence and discrete braking without wall penetration.
