# 0008: Public engine mechanisms and independent consumers

Historical decision. The current repository boundary is defined by [0009](0009-core-and-lab.md); application sources, tests and behavior guides now belong to Kinetra-Lab.


## Table of contents

- [Status and context](#status-and-context)
- [Decision](#decision)
- [Alternatives and consequences](#alternatives-and-consequences)
- [Verification](#verification)

## Status and context

Repository placement and package support superseded by [0009](0009-core-and-lab.md). Historical decision follows.

Accepted. Supersedes the application ownership of generic navigation in [0003](0003-sandbox-navigation-and-population.md) and amends session ownership in [0006](0006-variants-and-dotio-port.md). Gameplay policies in those decisions remain application-owned.

The checkout already had independent Simulation and Runtime targets. Demo still owned reusable geometric A*/route following; the launcher hardcoded application sessions and menu behavior. Runtime could only be enabled together with bundled applications. Documentation repeated build, controls and architecture across several entry points.

## Decision

Keep Simulation and Runtime as meaningful linking boundaries. Move body-aware Navigation and route following into the compiled simulation library with configurable resolution/clearance. Preserve Demo behavior and retain Dotio's compatibility graph/HideStar policies locally. Dotio vector calculations delegate to the canonical math API while its legacy names and coordinate conventions remain local.

Expose a runtime Session contract and direct runner. Put registered LaunchMenu, SessionHost and the standard event loop in an optional Launcher target, independent of concrete applications. Move concrete session adapters into their owning examples. Debug/help share layout and drawing; applications supply content/actions. Runtime, Launcher and bundled applications have separate CMake switches.

Require `.h`/`.cpp` for first-party C++, migrate includes and documentation, and enforce the rule in CTest. Generated GLAD/KHR are exempt. Put authoritative design/behavior pages under `docs/`; keep README concise and AGENTS focused on permanent engineering rules.

## Alternatives and consequences

A library per conceptual folder would increase target/dependency overhead without useful independent linking. An oversized AI framework would mix policy with mechanism. Rewriting Dotio's pixel graph and fear-biased search alongside ownership changes would increase behavior risk; its retained adapter boundary is explicit and covered by regressions.

The header migration is a source compatibility change: consumers must update includes. No compatibility `.hpp` shims are retained because they violate the new convention. There is no persisted format migration. Checkout `add_subdirectory` is supported; installed packages remain future work. Reverting ownership does not require saved-data migration.

Dotio remains in-tree, but a test copies it outside the engine source tree and builds its direct executable using public targets. A real Kinetra-Lab repository, i4sim integration and dependency/version policy are deferred. Static Dotio occupancy still allows only one active world.

## Verification

Existing application/physics/rendering regressions remain. New public navigation/math, generic launcher transition/failure and external-consumer tests cover the new boundaries. [Development](../development.md) owns reproducible commands and environment limitations. [Architecture](../architecture.md) owns the current dependency/ownership views.

Unused legacy Angle, Vec3 and Rect definitions were removed after checking all callers; live pixel/grid compatibility types remain application-local.
