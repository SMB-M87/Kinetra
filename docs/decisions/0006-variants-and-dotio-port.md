# Variants and Dotio boundaries

Historical decision. The current repository boundary is defined by [0009](0009-core-and-lab.md); application sources, tests and behavior guides now belong to Kinetra-Lab.


Ownership status: superseded in part by [0008](0008-public-engine-and-consumers.md). The original decision below remains historical; application behavior is maintained in the guides.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)
- [Navigation](#navigation)

## Context

The launcher selects Demo or Dotio and creates a fresh session with independent input, camera and application state. Dotio adds an Easy/Medium/Hard selection screen.

## Decision

A small Session interface owns concrete application adapters. Each adapter consumes shared debug-menu input before translating its variant controls. Dotio retains its game-specific AI, math compatibility, fuzzy decisions and A*/HideStar locally. Generic geometry, integration and contacts belong to Kinetra and are called through an explicit pixel/frame adapter.

## Consequences

The port retains source author attribution and does not depend on the upstream checkout. Updates and rendering run sequentially on the owning thread. M discards the current session. Dotio static raster storage permits one active world at a time; there are no resume slots. See the [Dotio guide](https://github.com/SMB-M87/Kinetra-Lab/blob/main/launcher/dotio/README.md).

## Navigation

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)
