# Kinetra Agent Guide

Permanent project rules for this repository. Explicit user instructions and higher-priority platform instructions take precedence. Read the current checkout before changing it.

## Table of contents

- [Identity and scope](#identity-and-scope)
- [Engineering rules](#engineering-rules)
- [Architecture boundaries](#architecture-boundaries)
- [Documentation](#documentation)
- [Verification](#verification)
- [Git and safety](#git-and-safety)
- [Functionality index](#functionality-index)

## Identity and scope

- Kinetra is a C++20 2D engine with a headless simulation library and optional SDL2/OpenGL 3.3 runtime.
- Kinetra contains only reusable core, core tests and development infrastructure.
- Launcher, Demo, Dotio and application tests live in Kinetra-Lab.
- Linux is supported; Artix/Linux is the desktop target. Do not imply untested platform support.
- Application behavior is documented in Kinetra-Lab.

## Engineering rules

- First-party C++ headers MUST use `.h` and implementations MUST use `.cpp`.
- Never introduce `.hpp`, `.hxx`, `.hh`, `.cc` or `.cxx` in first-party code.
- Third-party/generated source is exempt: leave `src/glad.c`, `include/glad/` and `include/KHR/` unchanged.
- Update includes, CMake, tests, scripts and documentation when moving files. Run the repository checker.
- Follow the root `.clang-format`; avoid unrelated formatting churn.
- Prefer RAII, const-correctness, explicit ownership, predictable lifetimes and standard C++20.
- Use small cohesive interfaces. Add abstraction only when it hides a concrete source of complexity.
- Preserve public contracts, coordinate units, timing and application behavior unless explicitly changing them.
- Validate untrusted input at boundaries and report actionable errors. Do not swallow exceptions.
- Explain non-obvious algorithms, invariants and workarounds in concise English comments/docstrings.
- Inspect nearby implementation, tests and documentation before editing.
- For substantial changes, state the intended boundary and affected areas before implementation.
- Do not introduce dependencies or new backend/framework choices without a concrete need.

## Architecture boundaries

- Reusable mechanisms belong in Kinetra; game-specific policies belong in applications.
- Math, physics, collision, spatial queries, navigation and steering must remain headless.
- Physics/math must not depend on SDL/OpenGL.
- Engine modules must not depend on Demo, Dotio or their private headers.
- External consumers use public `include/kinetra/` APIs and public `Kinetra::` CMake targets.
- Keep GLAD/OpenGL and SDL implementation types out of public engine interfaces.
- Keep world generation, AI decisions, combat, difficulty, spawning and bindings application-owned.
- Application fixed rates are policy: do not hardcode Demo or Dotio timing into the engine.
- Preserve headless and runtime-only builds, independently of examples and launcher.
- Launcher implementation belongs to Kinetra-Lab; core retains the direct Session contract.
- Overlays own shared layout/rendering/input capture; applications provide content and actions.
- Do not add a library for each directory. Targets must represent useful linking boundaries.
- Record meaningful boundary changes in an ADR and update the external-consumer test.
- Nested AGENTS files are only appropriate for genuinely different local rules; document their scope.

## Documentation

- Update relevant documentation whenever behavior, APIs, build configuration or architecture changes.
- Use concise technical English for code and documentation; communicate with the user in Dutch by default.
- README is the short introduction, quick start and navigation entry point.
- Use [docs/README.md](docs/README.md) to find each concept's authoritative page; link instead of duplicating explanations.
- Keep a useful explicit TOC below each maintained Markdown title/introduction and synchronize its links.
- Prefer Mermaid for architecture, ownership, interactions and lifecycle explanations.
- Every diagram must reflect implemented components, label relationships and explain its takeaway.
- Do not create diagrams for trivial functions or one page per class.
- Keep diagrams synchronized with code; never imply inheritance for enums or plain composition.
- Keep historical ADRs traceable. Mark obsolete decisions superseded and link their replacement.
- Validate relative links, referenced paths, examples and target names. Report Mermaid rendering limits honestly.
- Keep the capability-level index below concise; detailed navigation belongs in subsystem pages.

## Verification

- Build after every task changing Kinetra, including documentation/configuration tasks.
- Runnable applications are built and verified in Kinetra-Lab.
- Build headless simulation/tests and the optional SDL2/OpenGL runtime. Reconfigure after CMake changes.
- Run relevant behavioral tests; add regression coverage for changed behavior and important boundaries.
- Do not remove tests to hide regressions or describe untested behavior as verified.
- Run formatting and repository checks. Documentation-only changes need link/path/claim validation.
- Test public API/CMake boundaries with the external consumers when changing ownership or build configuration.
- Run graphics checks with an actual context (desktop, Xvfb or SDL offscreen); report the backend used.
- Smoke-test menu, Demo and Dotio after runtime changes. Preserve session input isolation and cleanup.
- Report exact commands, results, blockers and remaining checks. Software rendering does not verify desktop feel/performance.
- Authoritative commands and environment notes are in [development](docs/development.md).

## Git and safety

- Inspect `git status` before edits and review the final diff.
- Preserve unrelated changes. Never reset, clean, force-push or delete user data without authorization.
- Do not stage, commit or push unless explicitly requested; never infer permission from “finish” or “deliver”.
- Do not modify sibling repositories without permission. Treat them as read-only references.
- Keep credentials and private environment values out of output and files.
- Avoid network access and destructive operations unless necessary and authorized.

## Functionality index

| Capability | Implementation | Tests / authoritative documentation |
| --- | --- | --- |
| Math, collision, physics, spatial search | [public API](include/kinetra/), [simulation](src/simulation/) | [simulation tests](tests/simulation_tests.cpp), [contracts](docs/engine/math-physics.md) |
| Navigation and steering | [navigation](include/kinetra/navigation.h), [steering](include/kinetra/steering.h) | [navigation tests](tests/navigation_tests.cpp), [design](docs/engine/navigation.md) |
| Window, input, rendering and direct sessions | [runtime](src/engine/), [session API](include/kinetra/session.h) | [runtime tests](tests/runtime_tests.cpp), [runtime guide](docs/engine/runtime.md) |
| Shared overlays | [debug](include/kinetra/debug_view.h), [help](include/kinetra/help_view.h) | [design](docs/engine/launcher-overlays.md) |
| Applications and launcher | [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab) | Application-owned sources and tests |
| Build and repository validation | [CMake](CMakeLists.txt), [tools](tools/), [consumer](tests/consumer/) | [development](docs/development.md) |
