# Independent 2D engine and simulation modules

Historical decision. The current repository boundary is defined by [0009](0009-core-and-lab.md); application sources, tests and behavior guides now belong to Kinetra-Lab.


## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

## Context

Kinetra provides reusable C++20 simulation and an SDL2/OpenGL runtime on Linux. Demo and Dotio own their application policy. The simulation target depends only on the standard library.

## Decision

Circles and oriented rectangles use a uniform-grid broad phase, discrete linear/angular contact response, and fixed timesteps. Body motion modes select fixed, controlled or dynamic response. Gravity is optional. Engine geometry and steering are independently callable. Application rules remain in the examples; the original sibling repositories are not dependencies.

## Consequences

The self-developed solver supports the tested 2D contracts and simple contacts. It has discrete collision and single-contact manifold limits. Linux is the supported platform; no Windows/macOS or 3D support is claimed. Consumers link checkout CMake targets.
