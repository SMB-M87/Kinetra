# Headless simulation implementation

These sources form `Kinetra::Simulation` and depend only on the standard library. Their public contracts are declared in [include/kinetra](../../include/kinetra/README.md).

## Table of contents

- [Source map](#source-map)
- [Relationships](#relationships)

## Source map

| File | Responsibility |
| --- | --- |
| [physics.cpp](physics.cpp) | Body integration, contacts, impulses and world stepping |
| [spatial_grid.cpp](spatial_grid.cpp) | AABB indexing and overlap queries |
| [collision_grid.cpp](collision_grid.cpp) | Occupancy and segment collision queries |
| [navigation.cpp](navigation.cpp) | Path planning and route following |
| [steering.cpp](steering.cpp) | Local steering and obstacle avoidance |

## Relationships

Navigation and steering build on geometry and spatial queries without depending on SDL/OpenGL. Applications own world generation and AI policy.

See [math and physics](../../docs/engine/math-physics.md), [navigation](../../docs/engine/navigation.md) and the [test guide](../../tests/README.md). Return to the [source map](../README.md).
