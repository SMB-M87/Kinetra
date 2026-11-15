# Sandbox routes and player-active procedural agents

Historical decision. The current repository boundary is defined by [0009](0009-core-and-lab.md); application sources, tests and behavior guides now belong to Kinetra-Lab.


Ownership status: superseded in part by [0008](0008-public-engine-and-consumers.md). The original decision below remains historical; application behavior is maintained in the guides.

## Table of contents

**Navigation:**

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

**On this page:**

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

## Context

Demo combines continuous player and agent motion, exact free click targets, occupied-target projection and seeded wandering through loaded terrain.

## Decision

Application-owned eight-neighbor A* uses a one-unit grid, cached swept links and connected regions. Clearance uses a conservative 0.85-unit circular envelope against circles and rotated boxes. Occupied goals resolve to the nearest reachable free node; free endpoints remain exact. Route following shortens visible segments and looks ahead around corners. A body pushed into the planning margin can first depart outward from nearby surfaces before resuming its route.

## Consequences

Moving-body avoidance stays local. Commands expire after 20 active simulated seconds, and agents resume wandering on arrival. Agents freeze outside 1.75× maximum normal player view, clear paths and ignore goals. Beyond 1.25× maximum debug player view, their bodies are evicted and handles compacted. Current zoom and Ghost pan do not affect these bounds. Population is capped at 1,200; navigation at 262,144 cells. Conservative clearance can reject physically passable gaps; moving crowds can still prevent arrival.
