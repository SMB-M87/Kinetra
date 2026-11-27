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

Read only what the task needs; the pages are written to stand on their own. Where a row names sections, read those and not the whole page. Search the roadmap and the design index for the topic instead of reading them through.

| Task | Read |
| --- | --- |
| Every task | This file |
| Start, plan, review or hand over a change | The `feature` skill, and the section of the [workflow](docs/workflow.md) for the step you are at, which the skill links; the whole workflow only to change the cycle itself |
| Build or review a change, so known mistakes are not repeated | [Review lessons](docs/review-lessons.md) |
| Write or review C++ or CMake | [Conventions](docs/conventions.md) |
| Add tests | [Test kinds](docs/testing.md#test-kinds), [rules for tests](docs/testing.md#rules-for-tests) and [writing a doctest suite](docs/testing.md#writing-a-doctest-suite) |
| Pick a verification level | [Verification levels](docs/testing.md#verification-levels) and [choosing targeted tests](docs/testing.md#choosing-targeted-tests) (`verify` skill); [continuous integration](docs/testing.md#continuous-integration) only for a CI change; build options in [development](docs/development.md) |
| Write `///` comments | [Code comments](docs/documentation.md#code-comments) |
| Write a README or another page | [What goes where](docs/documentation.md#what-goes-where), the [module](docs/documentation.md#module-readme-template) or [skill README template](docs/documentation.md#skill-readme-template), and [page layout](docs/documentation.md#page-layout) |
| Draw a diagram | [Diagrams](docs/documentation.md#diagrams) |
| Work on one module | Its guide: [simulation](src/simulation/README.md), [runtime and launcher](src/runtime/README.md), [tests](tests/README.md) |
| Understand dependencies and ownership | [Architecture](docs/architecture.md), [engine contracts](docs/README.md#engine-contracts) |
| Find out why a boundary exists | [Decisions](docs/decisions/README.md) |
| See current and planned work, or record an idea | The [roadmap](docs/roadmap.md) items a search for the topic finds (`grep -n -i '<key word>' docs/roadmap.md`); the whole roadmap only to choose or order items (`roadmap` skill); a [design document](docs/design/README.md) only when an item links it |

## Identity and scope

- Kinetra is a C++20 2D engine: a headless simulation library plus an optional SDL2/OpenGL 3.3 runtime and launcher.
- Kinetra contains reusable core, core tests and development infrastructure only. Demo, Dotio, i4sim, the bundled launcher executable and application tests live in Kinetra-Lab.
- Kinetra is built from source on Linux. CI uses Ubuntu 24.04 LTS as the reference environment for compiler warnings, builds and tests. Document distribution-specific validation only when it has run.

## Development cycle

Every change follows the [workflow](docs/workflow.md). The implementing session plans the test cases, implements and verifies them, reviews its own final diff and fixes findings before pushing. A docs-only change records the work in the pull request description and skips the design document. The `feature` skill walks through it.

- Each branch has a design document in [docs/design/](docs/design/README.md), written before code; small changes get a short one. A docs-only change (only Markdown, `docs/` or `.agents/`) needs none: its pull request description carries the problem, the decisions and the verification. A change across Kinetra and Lab gets one design document, in the repository that holds most of it, which the other repository's pull request links.
- Design documents are history once implemented. Do not read the design index or older design documents to start a task: read your branch's own, and another only when the roadmap item, a README, an open point or the user links it, or a search of `docs/design/` for the topic finds it.
- Settle open, load-bearing decisions with the user before implementing; use the `grilling` skill when there are several.
- Record hard-to-reverse, surprising trade-offs as an ADR (`adr` skill).
- Separate refactoring commits from behavior changes.
- Keep the [roadmap](docs/roadmap.md) current: a branch moves its item to *Now* when it starts and removes it when it finishes, and follow-ups and the user's ideas become items (`roadmap` skill).

*Why:* the code grew feature by feature without recorded design or reasons. The cycle leaves a trail, so the next reader understands both what changed and why. The trail is for looking up a reason, not for reading at the start of every task: the design documents hold more text than the code.

## Verification by impact

Choose the lowest level that covers the change. Commands are listed in [testing](docs/testing.md#verification-levels).

| Level | When | What to run |
| --- | --- | --- |
| Docs | Only Markdown, comments, skills or agent rules changed | `python3 tools/check_docs.py`; no build |
| Targeted | Code inside one module; no public header, CMake or dependency change | Incremental build of the affected targets in the existing build directory, plus their CTest entries |
| Full | Public `include/kinetra/` header, CMake, dependency, multiple modules, runtime/launcher behavior, or the last commit of a branch | Locally the targeted builds and tests of every changed area; then the pull request's CI on Ubuntu 24.04 LTS (format, docs, `headless`, `ci` with the graphics cases on software OpenGL, consumer). Verified only when CI passes ([testing](docs/testing.md#full)) |

- Report the exact commands, results and blockers. Never call a check passed that did not run.
- A failed configure or build means verification is incomplete.
- Prove each goal with a check or run as wide as its wording, or narrow the wording. A goal that names a set (every header, every platform, in a window) needs a check over that whole set, not over the one member the change touched. This was a review lesson from #66, Kinetra-Lab#133 and #67; the user made it a rule on 2026-11-27.

*Why:* a full build takes minutes and proves nothing for a README edit. Skipping compilation after a code change hides errors until much later. CI runs the graphics cases on the same software backend as a local run, so the local machine adds nothing to the Full level but a playtest on a desktop. A goal proven by a narrower check than its wording is not proven: #66 searched for one moved header where its goal named all of `runtime/`, and #67 promised a read-back that works in a window while CI runs only offscreen.

## Git

- Work on a branch named `<type>/<topic>`, using the conventional-commit types (`feat`, `fix`, `refactor`, `test`, `docs`, `build`, `chore`). Never commit to `main`.
- Commit freely on that branch, in small logical steps. Use the message format `type(scope): imperative summary` in English, with an optional body explaining why.
- Stage explicit paths. Never use `git add -A`; preserve unrelated work and inspect `git status` before and after.
- When the branch is complete (design document `implemented`, verification passed), push the branch and open a pull request against `main`, or against the branch below it in a stack. Use the design document's *Pull request* section as the description; a docs-only change, which has no design document, writes it directly ([workflow](docs/workflow.md#8-commit-push-and-open-a-pull-request)).
- Work in stacked pull requests where possible: when a goal splits into steps that can each be reviewed on their own, give every step its own branch and pull request, based on the step below it, so the user gets small increments toward the same goal. When a lower step changes, merge it into the branches above it; the stack merges bottom-up ([stacked pull requests](docs/workflow.md#stacked-pull-requests)).
- Never push to `main`, force-push, rebase published history, tag or change remotes. Merge a pull request only under the merge rule below.
- The author reviews its own change in the implementing session ([workflow](docs/workflow.md#6-review)). It fixes findings directly on the branch and records evidence in the design document where applicable and in the pull request description. An independent `pr-review` is available on the user’s request.
- The author follows its pull request and uses `pr-feedback` to handle actual review feedback.
- A review fix carries everything that names what it changes. Before pushing it, the author searches the key words of each changed part and updates every hit in the same push: every section of the design document (*Decisions*, *Design*, *Affected areas*, *Test plan*, *Verification*, *Review* and *Pull request*), or for a docs-only pull request its description; the live pull request description; and, for a change across Kinetra and Lab, the other repository's copy of the same job, script, page or claim, in both directions. These were review lessons from #33 and #37 and Kinetra-Lab#78 and Kinetra-Lab#83; they became a rule after they came back in most rounds of #37 and Kinetra-Lab#83.
- **Merge rule.** The author merges once its final review is complete, the required GitHub checks are green on the current head, GitHub reports no conflict, all review threads are resolved, and every prerequisite pull request has merged. If the user requested an independent review before merging, satisfy it as well. Merge with a merge commit, bottom-up for stacks and Kinetra before Lab. A pointer-only pull request follows the automation rule below. The user checks merged work afterwards, including any playtest assigned to them.
- If `git push` over SSH is unavailable, push once over HTTPS with the GitHub CLI credentials: `git -c credential.helper= -c credential.helper='!gh auth git-credential' push https://github.com/<owner>/<repo>.git <branch>`. Never store credentials or change the remote or global git configuration for this.
- Kinetra-Lab tracks Kinetra as a submodule. For an engine change used by Lab, commit and open the Kinetra pull request first, then commit the pointer in Lab. The Lab pull request links the Kinetra one and states that Kinetra merges first.
- **Docs-only pull requests.** Changes limited to Markdown, `docs/` or `.agents/` use the Docs verification level and the author’s review. Lab’s [verification rules](https://github.com/SMB-M87/Kinetra-Lab/blob/main/AGENTS.md#verification-in-lab) cover documentation and submodule paths that CI handles differently.
- **Pointer-only pull requests.** A Kinetra-Lab pull request that changes nothing but the `external/Kinetra` pointer, to a commit on Kinetra `main`, follows this automation. Kinetra's `lab-pointer` workflow opens it after each merge on Kinetra `main`, and hourly to catch up, and turns on auto-merge, so GitHub merges it once its CI passes. It waits while another Lab pull request changes the pointer, so that one merges first with the Kinetra commit it was verified with ([tool](tools/README.md#lab_pointersh)). An agent opens or merges one by hand only when the workflow failed or is turned off. The move to the Kinetra merge commit on `main` always goes in such a pull request of its own, never as a new commit on the Lab pull request that is open or reviewed, so that pull request does not run CI again for it. Following a Kinetra *branch* that changes during review stays a commit on the Lab branch, as the [stack rules](docs/workflow.md#stacked-pull-requests) say ([changes across Kinetra and Lab](docs/workflow.md#changes-across-kinetra-and-lab)).

*Why:* the author can fix and test findings while it still has the implementation context. CI checks the complete pushed change, and the pull request records the evidence and the dependency order.

## Engineering rules

Details and examples are in the [conventions](docs/conventions.md).

- First-party headers use `.h` and implementations use `.cpp`; never `.hpp`, `.hxx`, `.hh`, `.cc` or `.cxx`. Headers live only under an include root, in the folder of their layer; the [directory layout](docs/conventions.md#directory-layout) applies to Kinetra and Lab. Leave third-party/generated files unchanged (`src/glad.c`, `include/glad/`, `include/KHR/`).
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
- Format, examples and a before/after sample are in the [documentation guide](docs/documentation.md#code-comments). `TrafficGate` in [traffic.h](include/kinetra/simulation/traffic.h) is a good existing example.

*Why:* the user wants to learn the code from the code. clangd shows `///` on hover, and Doxygen can render it later without rewriting.

## Repository documentation

- Each module has one README with real content: purpose, concepts, a diagram, key types, data flow, invariants, how to change it, tests and open points. Modules are engine subsystems, applications, the launcher and test suites. Use the [module template](docs/documentation.md#module-readme-template).
- A subdirectory gets its own README only when it has content worth reading on its own. Third-party wrappers keep their provenance READMEs.
- `docs/` holds cross-module material: architecture, contracts, decisions, design documents and the [roadmap](docs/roadmap.md) of planned work ([format](docs/documentation.md#roadmap)).
- Every concept has one authoritative place; link to it for depth. A link never replaces an explanation the local reader needs.
- When a change alters a rule, procedure or term that more than one page states, search for its old wording, its key words and the names of its parts (`python3 tools/check_copies.py --term '<old wording>' --term '<key word>' --term '<part>'`, in Kinetra-Lab `python3 external/Kinetra/tools/check_copies.py`, which also covers `external/Kinetra`; it searches case-insensitively, and lists the CI jobs, CMake options, presets, flags and inline names the diff removes and where they are still named), read every hit, and change every copy in the same commit, or in the fix commit when review finds a missed copy. The parts are what the rule is made of or acts on: when a change exempts a case from an artifact or a step, they are also the artifact's sections, the steps that fill them and the phrases that presuppose it (for the docs-only exemption: *Decisions*, *Test plan*, *Verification* and "is the record", not only "design document"). Every copy then says the same. Then search each key word that the change moves, renames or makes stale on its own (a label, a key, a menu row, a flag) with `git grep --recurse-submodules -n -i '<word>'` over the repository, which in Kinetra-Lab also covers `external/Kinetra` (plain `git grep` skips submodules), and read every hit: `check_copies.py` reads only Markdown and YAML and finds a phrase only in the word order given, so it misses code, CMake and script comments and sentences that word the same thing otherwise. This was a review lesson from #26; it became a rule after it came back in #27, the script does the mechanical part since it came back in #30 and #35, and the parts joined the rule after #37 still missed copies in its first three rounds. The `git grep` step was Kinetra-Lab's lesson from Kinetra-Lab#147; the user made it a rule on 2026-11-27, after it came back in Kinetra-Lab#148.
- **Media.** The GIFs in [docs/media](docs/media/README.md) show the shared UI, rendered from Kinetra-Lab's Demo by Lab's capture tool. A pull request that changes what a capture shows, in either repository, refreshes in the same pull request the captures that a media guide's table names for the change, and commits no other: [Kinetra's](docs/media/README.md#which-gifs-a-change-makes-stale) for the shared UI, [Kinetra-Lab's](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/media/README.md#which-captures-a-change-makes-stale) for the applications. The captures of a change here are rendered in Kinetra-Lab with its pointer on the Kinetra branch, and committed on that branch. The same capture differs in a few pixels between machines, so a capture that no change touched stays as it is.
- Diagrams show implemented code only; each one states its takeaway.
- Layout, table of contents and navigation follow the [documentation guide](docs/documentation.md#page-layout).
- End a page with **Open points** when known gaps or questions exist. Replace outdated text instead of appending to it.
- Write documentation and comments in English; talk to the user in Dutch.

*Why:* a README in every directory produces navigation without insight. Depth belongs where the reader looks for it.

## Skills

Repository skills live in [.agents/skills/](.agents/skills/README.md); `.claude/skills` links there. The global skill set may add book-based references (refactoring, clean architecture, …), `teach` and `code-review`. Repository skills do not duplicate them. Skills and these rules stay agent-neutral: no model names, and an agent-specific command or tool only as an example next to the action it performs, so Codex and other agents can follow them as well as Claude Code.

| Skill | Use |
| --- | --- |
| `feature` | Start and run a change through the full cycle |
| `grilling` | Settle open decisions with the user in rounds of forms (on request or from `feature`) |
| `verify` | Pick and run the verification level for the current diff |
| `doc-module` | Write or deepen a module README and its header `///` summaries |
| `refactor` | Behavior-preserving restructuring in named, tested steps (on request) |
| `adr` | Record an architecture decision |
| `pr-feedback` | Process review feedback on an open pull request: commits, push, a reply per comment (on request, and for agent reviews in the author's session) |
| `pr-review` | Independently review a pull request or stack on the user’s request |
| `roadmap` | Keep the backlog in `docs/roadmap.md`: brainstorm ideas, pick up, close items (on request or from `feature` and `grilling`) |

- Each skill directory holds a `SKILL.md` for the agent and a `README.md` with use cases for contributors. Follow the [skill README template](docs/documentation.md#skill-readme-template) and link the guide from the [skills overview](.agents/skills/README.md).
- A change that adds or changes a skill updates its README and the overview in the same commit.

*Why:* `SKILL.md` is written as instructions for an agent. A person choosing a skill, or reviewing what it did, needs examples of when to use it and what it changes.

## Functionality index

| Capability | Implementation | Tests / authoritative documentation |
| --- | --- | --- |
| Math, collision, physics, spatial search | [public API](include/kinetra/), [simulation](src/simulation/) | [simulation tests](tests/simulation_tests.cpp), [contracts](docs/engine/math-physics.md) |
| Navigation, steering, avoidance, traffic | [navigation](include/kinetra/simulation/navigation.h), [steering](include/kinetra/simulation/steering.h), [traffic](include/kinetra/simulation/traffic.h) | [navigation tests](tests/navigation_tests.cpp), [design](docs/engine/navigation.md) |
| Window, input, rendering and direct sessions | [runtime](src/runtime/), [session API](include/kinetra/runtime/session.h) | [runtime tests](tests/runtime_tests.cpp), [runtime guide](docs/engine/runtime.md) |
| Shared overlays and text | [settings](include/kinetra/runtime/settings_view.h), [debug](include/kinetra/runtime/debug_view.h), [help](include/kinetra/runtime/help_view.h), [text](include/kinetra/runtime/bitmap_text.h) | [settings tests](tests/settings_view_tests.cpp), [presentation tests](tests/presentation_tests.cpp), [contracts](docs/engine/launcher-overlays.md) |
| Generic launcher | [launcher API](include/kinetra/runtime/launcher.h) | [tests](tests/launcher_tests.cpp), [contracts](docs/engine/launcher-overlays.md) |
| Applications and bundled launcher | [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab) | Application-owned registration, executables and tests |
| Build, presets, CI and docs checks | [CMake](CMakeLists.txt), [presets](CMakePresets.json), [CMake helpers](cmake/), [CI](.github/workflows/ci.yml), [Lab pointer](.github/workflows/lab-pointer.yml), [consumer](tests/consumer/), [tools](tools/README.md) | [development](docs/development.md), [testing](docs/testing.md) |
