# 0009: Separate core library from Lab consumers

## Table of contents

- [Decision](#decision)
- [Consequences](#consequences)
- [Verification](#verification)

## Decision

Accepted. Supersedes the repository placement and deferred installed-package support in [0008](0008-public-engine-and-consumers.md).

Kinetra owns Simulation, Runtime, public core headers, core tests and development tools.
Kinetra-Lab owns Launcher (including its generic host/menu implementation), Demo, Dotio, application tests and behavior guides. Shared debug/help overlays and the direct Session runner remain reusable core features.

Lab consumes public Kinetra targets via an explicit source path, installed package, or sibling fallback. No submodules, automatic downloads or engine-source duplication are used.

## Consequences

The core no longer exports `Kinetra::Launcher` or installs `kinetra/launcher.h`; Lab preserves both names for its launcher consumers. Simulation and Runtime APIs are unchanged. Former bundled application options and executables move out of the core build. Clean build directories avoid stale application binaries.

CMake install/export rules provide `Kinetra::Simulation` and optional `Kinetra::Runtime`. Runtime packages discover SDL2/OpenGL dependencies. There is no saved-data migration or binary ABI promise. Historical ADRs remain in core for traceability, with links to relocated source.

## Verification

Core is configured/built/tested independently. Lab is checked using source and installed integration, including headless logic, graphical builds and bounded application runs when dependencies/context are available. See [development](../development.md) and the [Lab guide](https://github.com/SMB-M87/Kinetra-Lab).
