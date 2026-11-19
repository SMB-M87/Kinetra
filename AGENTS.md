# Kinetra Agent Guide

Permanent rules for agents and developers working on Kinetra. They add to the user's global engineering instructions; where this file is more specific, it wins. Kinetra-Lab reuses these rules and adds only application-specific rules in its own `AGENTS.md`. Explicit user instructions take precedence over both.

Each rule states *why* it exists. Use the reason to decide edge cases instead of applying the wording mechanically.

## Table of contents

- [Where to read what](#where-to-read-what)
- [Identity and scope](#identity-and-scope)
- [Development cycle](#development-cycle)
- [Verification by impact](#verification-by-impact)
- [Git](#git)
- [Engineering rules](#engineering-rules)
- [Architecture boundaries](#architecture-boundaries)
- [Code documentation](#code-documentation)
- [Repository documentation](#repository-documentation)
- [Skills](#skills)
- [Functionality index](#functionality-index)

[Project overview](README.md) · [Documentation index](docs/README.md)

## Where to read what

Read only what the task needs; the pages are written to stand on their own.

| Task | Read |
| --- | --- |
| Every task | This file |
| Start, plan, review or hand over a change | [Workflow](docs/workflow.md) |
| Write or review C++ or CMake | [Conventions](docs/conventions.md) |
| Add tests or pick a verification level | [Testing](docs/testing.md); build options in [development](docs/development.md) |
| Write READMEs, diagrams or `///` comments | [Documentation guide](docs/documentation.md) |
| Work on one module | Its guide: [simulation](src/simulation/README.md), [runtime and launcher](src/engine/README.md), [tests](tests/README.md) |
| Understand dependencies and ownership | [Architecture](docs/architecture.md), [engine contracts](docs/README.md#engine-contracts) |
| Find out why a boundary exists | [Decisions](docs/decisions/README.md) |
| See current and planned work | [Design documents](docs/design/README.md) |

## Identity and scope

- Kinetra is a C++20 2D engine: a headless simulation library plus an optional SDL2/OpenGL 3.3 runtime and launcher.
- Kinetra contains reusable core, core tests and development infrastructure only. Demo, Dotio, i4sim, the bundled launcher executable and application tests live in Kinetra-Lab.
- Linux is supported; Artix/Linux is the desktop target. Do not imply untested platform support.

## Development cycle

Every change follows the full cycle in the [workflow](docs/workflow.md): **understand → design → test plan → implement → verify → review → document → commit**. The `feature` skill walks through it.

- Each branch has a design document in [docs/design/](docs/design/README.md), written before code. Small changes get a short document, but they still get one.
- Settle open, load-bearing decisions with the user before implementing; use the `grilling` skill when there are several.
- Record hard-to-reverse, surprising trade-offs as an ADR (`adr` skill).
- Separate refactoring commits from behavior changes.

*Why:* the code grew feature by feature without recorded design or reasons. The cycle leaves a trail, so the next reader understands both what changed and why.

## Verification by impact

Choose the lowest level that covers the change. Commands are listed in [testing](docs/testing.md#verification-levels).

| Level | When | What to run |
| --- | --- | --- |
| Docs | Only Markdown, comments, skills or agent rules changed | `python3 tools/check_docs.py`; no build |
| Targeted | Code inside one module; no public header, CMake or dependency change | Incremental build of the affected targets in the existing build directory, plus their CTest entries |
| Full | Public `include/kinetra/` header, CMake, dependency, multiple modules, runtime/launcher behavior, or the last commit of a branch | Full core build and tests, the Lab integration build (`./build.sh` in Lab), and graphics checks for runtime changes |

- Report the exact commands, results and blockers. Never call a check passed that did not run.
- A failed configure or build means verification is incomplete.

*Why:* a full build takes minutes and proves nothing for a README edit. Skipping compilation after a code change hides errors until much later.

## Git

- Work on a branch named `<type>/<topic>`, using the conventional-commit types (`feat`, `fix`, `refactor`, `test`, `docs`, `build`, `chore`). Never commit to `main`.
- Commit freely on that branch, in small logical steps. Use the message format `type(scope): imperative summary` in English, with an optional body explaining why.
- Stage explicit paths. Never use `git add -A`; preserve unrelated work and inspect `git status` before and after.
- When the branch is complete (design document `implemented`, verification passed), push the branch and open a pull request against `main`. Use the design document's *Pull request* section as the description ([workflow](docs/workflow.md#8-commit-push-and-open-a-pull-request)).
- Never push to `main`, merge pull requests, force-push, rebase published history, tag or change remotes. The user reviews and merges.
- If `git push` over SSH is unavailable, push once over HTTPS with the GitHub CLI credentials: `git -c credential.helper= -c credential.helper='!gh auth git-credential' push https://github.com/<owner>/<repo>.git <branch>`. Never store credentials or change the remote or global git configuration for this.
- Kinetra-Lab tracks Kinetra as a submodule. For an engine change used by Lab, commit and open the Kinetra pull request first, then commit the pointer in Lab. The Lab pull request links the Kinetra one and states that Kinetra merges first.

*Why:* pull requests make agent work reviewable on GitHub, with the reasons attached. Merging stays a human decision.

## Engineering rules

Details and examples are in the [conventions](docs/conventions.md).

- First-party headers use `.h` and implementations use `.cpp`; never `.hpp`, `.hxx`, `.hh`, `.cc` or `.cxx`. Leave third-party/generated files unchanged (`src/glad.c`, `include/glad/`, `include/KHR/`).
- Follow `.clang-format`. Format the files you change; put any repository-wide reformatting in a separate commit and list it in `.git-blame-ignore-revs`.
- Code compiles warning-free with `KINETRA_WARNINGS`; CI fails on any Kinetra warning. Fix the cause (explicit conversions, renamed shadows) instead of suppressing it.
- Prefer RAII, const-correctness, explicit ownership, value types and standard C++20. Avoid global mutable state.
- Simulation results must be deterministic for a given seed: inject seeded RNGs; never use `std::rand` or `std::random_device` in simulation logic.
- Name units and tuning constants (`constexpr`) instead of repeating magic numbers.
- When a type has more than two related boolean flags, model them as an explicit state (`enum class` plus transitions).
- Keep functions focused. A function with commented sections, more than about 60 lines, or boolean flag parameters is a candidate for extraction.
- Use design patterns only when they remove demonstrated duplication or variation. Add abstraction only where it hides real complexity.
- Validate untrusted input at boundaries and report actionable errors. Do not swallow exceptions.
- Preserve public contracts, coordinate units, timing and application behavior unless the design document changes them.
- Introduce no dependency or backend without a concrete need recorded in the design document.

## Architecture boundaries

- Reusable mechanisms belong in Kinetra; game-specific policies belong in applications.
- Math, physics, collision, spatial queries, navigation and steering stay headless; they never depend on SDL or OpenGL.
- Engine modules never depend on Lab applications or their headers.
- External consumers use the public `include/kinetra/` headers and the `Kinetra::` CMake targets. Keep GLAD/OpenGL and SDL types out of public interfaces.
- World generation, AI decisions, combat, difficulty, spawning, bindings and fixed rates are application policy.
- The reusable launcher menu, session hosting and overlays belong to `Kinetra::Launcher` and the runtime. Overlays own shared layout, rendering and input capture; applications supply content and actions.
- Targets represent useful linking boundaries; do not add a library per directory.
- Record boundary changes in an ADR and update the external-consumer test.
- Nested `AGENTS.md` files are only for genuinely different local rules and must state their scope.

## Code documentation

The code must explain itself to a reader who has not seen the design.

- Every public type, function and non-trivial data member in a header gets a `///` summary. Every enum whose values are not self-explanatory gets one too.
- The summary states the contract: purpose; units and coordinate space; ownership and lifetime of borrowed objects; preconditions; invariants; behavior on invalid input; and thread-safety when relevant.
- In `.cpp` files, explain *why* at each algorithm, invariant, workaround and tuning constant. Do not narrate what a line does.
- Update or remove comments in the same commit as the behavior they describe.
- Format, examples and a before/after sample are in the [documentation guide](docs/documentation.md#code-comments). `TrafficGate` in [traffic.h](include/kinetra/traffic.h) is a good existing example.

*Why:* the user wants to learn the code from the code. clangd shows `///` on hover, and Doxygen can render it later without rewriting.

## Repository documentation

- Each module has one README with real content: purpose, concepts, a diagram, key types, data flow, invariants, how to change it, tests and open points. Modules are engine subsystems, applications, the launcher and test suites. Use the [module template](docs/documentation.md#module-readme-template).
- A subdirectory gets its own README only when it has content worth reading on its own. Third-party wrappers keep their provenance READMEs.
- `docs/` holds cross-module material: architecture, contracts, decisions and design documents.
- Every concept has one authoritative place; link to it for depth. A link never replaces an explanation the local reader needs.
- Diagrams show implemented code only; each one states its takeaway.
- Layout, table of contents and navigation follow the [documentation guide](docs/documentation.md#page-layout).
- End a page with **Open points** when known gaps or questions exist. Replace outdated text instead of appending to it.
- Write documentation and comments in English; talk to the user in Dutch.

*Why:* a README in every directory produces navigation without insight. Depth belongs where the reader looks for it.

## Skills

Repository skills live in [.agents/skills/](.agents/skills/README.md); `.claude/skills` links there. The global skill set may add book-based references (refactoring, clean architecture, …), `teach` and `code-review`. Repository skills do not duplicate them.

| Skill | Use |
| --- | --- |
| `feature` | Start and run a change through the full cycle |
| `grilling` | Settle open decisions with the user in rounds of forms (on request or from `feature`) |
| `verify` | Pick and run the verification level for the current diff |
| `doc-module` | Write or deepen a module README and its header `///` summaries |
| `refactor` | Behavior-preserving restructuring in named, tested steps (on request) |
| `adr` | Record an architecture decision |
| `pr-feedback` | Process review feedback on an open pull request: commits, push, a reply per comment (on request) |

- Each skill directory holds a `SKILL.md` for the agent and gets a `README.md` that explains the skill to people with use cases ([skill README template](docs/documentation.md#skill-readme-template)); the READMEs are still being written ([open point](.agents/skills/README.md#open-points)). The [skills overview](.agents/skills/README.md) lists all skills.
- A change that adds or changes a skill updates its README and the overview in the same commit.

*Why:* `SKILL.md` is written as instructions for an agent. A person choosing a skill, or reviewing what it did, needs examples of when to use it and what it changes.

## Functionality index

| Capability | Implementation | Tests / authoritative documentation |
| --- | --- | --- |
| Math, collision, physics, spatial search | [public API](include/kinetra/), [simulation](src/simulation/) | [simulation tests](tests/simulation_tests.cpp), [contracts](docs/engine/math-physics.md) |
| Navigation, steering, avoidance, traffic | [navigation](include/kinetra/navigation.h), [steering](include/kinetra/steering.h), [traffic](include/kinetra/traffic.h) | [navigation tests](tests/navigation_tests.cpp), [design](docs/engine/navigation.md) |
| Window, input, rendering and direct sessions | [runtime](src/engine/), [session API](include/kinetra/session.h) | [runtime tests](tests/runtime_tests.cpp), [runtime guide](docs/engine/runtime.md) |
| Shared overlays and text | [settings](include/kinetra/settings_view.h), [debug](include/kinetra/debug_view.h), [help](include/kinetra/help_view.h), [text](include/kinetra/bitmap_text.h) | [settings tests](tests/settings_view_tests.cpp), [presentation tests](tests/presentation_tests.cpp), [contracts](docs/engine/launcher-overlays.md) |
| Generic launcher | [launcher API](include/kinetra/launcher.h) | [tests](tests/launcher_tests.cpp), [contracts](docs/engine/launcher-overlays.md) |
| Applications and bundled launcher | [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab) | Application-owned registration, executables and tests |
| Build, presets, CI and docs checks | [CMake](CMakeLists.txt), [presets](CMakePresets.json), [CMake helpers](cmake/), [CI](.github/workflows/ci.yml), [consumer](tests/consumer/), [tools](tools/README.md) | [development](docs/development.md), [testing](docs/testing.md) |
