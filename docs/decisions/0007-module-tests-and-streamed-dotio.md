# Module tests and streamed Dotio world

Historical decision. The current repository boundary is defined by [0009](0009-core-and-lab.md); application sources, tests and behavior guides now belong to Kinetra-Lab.


## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

## Context

Reusable engine targets live at the root. Demo, Dotio and the launcher own their CMake definitions and tests. A playable Dotio session uses bounded collision/navigation residency while exploring seeded connected passages.

## Decision

A 155-cell local window rebases in 16-cell steps. Actors, camera, projectiles and props shift together; paths and partitions are rebuilt. Absolute district coordinates preserve terrain on revisits. Enemies act within 90% of maximum debug player view and retire beyond 125%. Placement grows from 175% normal view to the activity rectangle over five active simulated minutes. The complete enemy body remains outside 110% normal view at spawn. Local density alone controls fuzzy pressure; Easy/Medium/Hard enemies take 1/2/3 player projectile hits.

## Consequences

The pool has 666 enemy slots. Retirement clears paths and partitions; reused enemies regain full durability. Death resets survival progression and retires live enemies, preserving difficulty. Props retain displacement while loaded and regenerate at their seeded spawn after eviction. Shared engine operations implement contacts, impulses and sweeps; combat and rewards stay local. Actor/prop contacts are discrete. Projectile hits use linear relative target motion and are ordered against walls and props. There is no disk persistence.
