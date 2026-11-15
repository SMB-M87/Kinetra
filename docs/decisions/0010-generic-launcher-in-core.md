# Generic launcher in Kinetra

## Table of contents

- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Decision

Supersede the generic launcher portion of [0009](0009-core-and-lab.md). Kinetra owns `kinetra/launcher.h`, its implementation and generic tests. `Kinetra::Launcher` builds with `KINETRA_BUILD_RUNTIME` and depends only on `Kinetra::Runtime`.

## Consequences

Lab consumes the public target and owns concrete application registration, CLI parsing, executables and application integration tests. Public API names and behavior remain unchanged. Headless builds remain independent of SDL/OpenGL and Lab. External consumer coverage exercises the launcher header and linking without a window.
