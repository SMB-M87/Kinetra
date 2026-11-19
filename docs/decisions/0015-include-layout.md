# 0015: Include layout by layer

Status: accepted. Public headers live in `include/kinetra/simulation/`, `include/kinetra/view/` and `include/kinetra/runtime/`; Kinetra and Kinetra-Lab share one directory rule.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

All 21 public headers sat in `include/kinetra/`. The layer rule (logic depends on `Kinetra::Simulation`, presentation on `Kinetra::Runtime`) could not be read from an include line. Lab's headless logic included `debug_menu.h`, which included `engine.h` and with it the `Runtime` declaration, only for the plain `Key`, `Input` and `Color` types. Kinetra-Lab had no directory rule either: headers in `src/` and `tests/`, one-file `include`/`src` pairs, and `../../` includes between modules. Evidence and the full design: [include layout](../design/2026-11-20-include-layout.md).

## Decision

- Group the public headers by layer: `simulation/` (`Kinetra::Simulation`), `view/` (header-only camera, menu-row and input types usable headless) and `runtime/` (`Kinetra::Runtime` and `Kinetra::Launcher`). `src/engine/` becomes `src/runtime/`.
- Move `Key`, `Input` and `Color` from `engine.h` to `view/input.h`.
- Write one [directory rule](../conventions.md#directory-layout) for both repositories: one `include/<name>/` and one `src/` per module, topic folders past about twelve headers, internal headers in `detail/`, test helpers in `tests/include/<name>/testing/`.
- Break the old paths once, without forwarding headers.

Rejected alternatives:
- **Two folders by target (`simulation/`, `runtime/`).** A pure move, but Lab's logic would include `<kinetra/runtime/...>` for header-only types it uses without linking the runtime, which reads as a layer violation.
- **Headless view types in `simulation/`.** Matches the target, but makes "simulation" cover menu rows and keyboard keys.
- **Forwarding headers at the old paths.** Lets Lab update separately, but the old paths stay valid and have to be removed again later; the only consumers update in the same round anyway.

## Consequences

- **Easier.** An include line shows its layer; a review spots logic that reaches into `runtime/`. New modules in either repository follow one written rule.
- **Harder.** Every `#include <kinetra/...>` in Kinetra and Lab changes in one round. A header that changes layer changes its path, which is a breaking change for consumers.
- **Lab.** Kinetra merges first with a merge commit; Lab's branch records it and updates its includes in the same pull request.
