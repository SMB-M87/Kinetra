# 0009: Separate core library from Lab consumers

## Table of contents

- [Decision](#decision)
- [Consequences](#consequences)
- [Verification](#verification)

## Decision

The generic launcher ownership below is superseded by [0010](0010-generic-launcher-in-core.md).

Accepted; revised to remove package installation and repository scripts. Supersedes the repository placement and deferred installed-package support in [0008](0008-public-engine-and-consumers.md).

Kinetra owns Simulation, Runtime, public core headers, core tests and development tools.
Kinetra-Lab owns Launcher (including its generic host/menu implementation), Demo, Dotio, application tests and behavior guides. Shared debug/help overlays and the direct Session runner remain reusable core features.

Lab consumes public Kinetra source targets with `add_subdirectory`. Its build defaults to `external/Kinetra`, the intended submodule location, and accepts `KINETRA_SOURCE_DIR` for an explicit checkout. Lab records the engine revision through that Git submodule. Lab documentation lives under `docs/`; Demo and Dotio source lives under `launcher/demo/` and `launcher/dotio/`. No package discovery or automatic downloads are used.

## Consequences

The core no longer exports `Kinetra::Launcher` or owns `kinetra/launcher.h`; Lab preserves both names for its launcher consumers. Simulation and Runtime APIs are unchanged. Former bundled application options and executables move out of the core build. Clean build directories avoid stale application binaries.

Source integration provides `Kinetra::Simulation` and optional `Kinetra::Runtime`. Install/export rules and repository-check scripts are removed; core behavioral tests and the external-consumer fixture remain. There is no saved-data migration or binary ABI promise. Historical ADRs remain in core for traceability, with links to relocated source.

## Verification

Core is configured/built/tested independently. Lab is checked using source integration, including headless logic, graphical builds and bounded application runs when dependencies/context are available. See [development](../development.md) and the [Lab guide](https://github.com/SMB-M87/Kinetra-Lab).
