# Seeded procedural terrain residency

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

## Context

Demo uses a seeded expandable world with repeatable obstacle geometry and bounded residency. The camera follows the player by default and supports a separate Ghost view.

## Decision

Each 32-unit terrain chunk derives its geometry independently from the seed and signed coordinate. Physics terrain is retained around the player at 1.25× maximum debug view; zoom and Ghost pan do not change that footprint. Ghost presentation uses a separate cached Terrain covering its visible view. It creates no distant physics or population.

## Consequences

Eviction forgets terrain and agent state. Terrain geometry regenerates identically on return, while population can be newly generated. On resident terrain/population changes, the scene restores compacted movers before appending fixed obstacles and remaps body handles. Storage is bounded by the view and population caps, without world saves. See [population policy](0003-sandbox-navigation-and-population.md).
