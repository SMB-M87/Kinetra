# Documentation guide

How Kinetra and Kinetra-Lab are documented. Documentation exists to give insight into the purpose of the code, how it works and why it works that way. Navigation is only the means to reach that insight.

## Table of contents

- [What goes where](#what-goes-where)
- [Module README template](#module-readme-template)
- [Code comments](#code-comments)
- [Diagrams](#diagrams)
- [Page layout](#page-layout)
- [Style and maintenance](#style-and-maintenance)
- [Open points](#open-points)

[Directory guide](README.md) · [Project overview](../README.md) · [Documentation index](README.md) · [Agent rules](../AGENTS.md)

## What goes where

| Question a reader has | Where the answer lives |
| --- | --- |
| What is this project and how do I start it? | Root `README.md`: introduction, quick start, links |
| What does this module do, how does it work, how do I change it? | The module README ([template](#module-readme-template)) |
| What exactly does this function or type promise? | `///` summary in the header |
| Why is this line or algorithm written this way? | *Why*-comment in the `.cpp` |
| How do modules depend on each other; who owns what? | `docs/architecture.md` |
| What is the contract of an engine subsystem? | `docs/engine/*.md` |
| Why was a boundary chosen? | ADR in `docs/decisions/` |
| What was changed in a branch, and why? | Design document in `docs/design/` |
| How do we work? | `AGENTS.md`, [workflow](workflow.md), [conventions](conventions.md), [testing](testing.md) and this page |

Modules are engine subsystems (simulation, runtime, launcher), Lab applications (Demo, Dotio, i4sim), the bundled launcher and test suites. A subdirectory such as `include/`, `src/` or `tests/` gets its own README only when it has something to say beyond its parent, for example a non-obvious file layout or a contract of its own. Third-party directories keep a short provenance README.

## Module README template

```markdown
# <Module name>

<One paragraph: what this module is for and who uses it.>

## Table of contents
<local heading links>

[Parent guide](../README.md) · [Project overview](<root>/README.md) · [Documentation index](<root>/docs/README.md)

## Purpose and scope
The problem it solves. What it deliberately does not do, and where that lives instead.

## Concepts
| Term | Meaning |
Domain vocabulary, including units and coordinate spaces.

## Structure
A component or class diagram, with one sentence on what it shows.
| File | Responsibility |

## How it works
Data flow and lifecycle (sequence or state diagram). The key algorithms,
with the reasoning and the trade-offs that were considered.

## Invariants and contracts
What must always hold: ownership, units, determinism, ordering.

## Changing this module
Recipes such as "to add X, change Y and Z, and test with W". Known pitfalls.

## Tests
What is covered and what is not, and the commands to run them.

## Open points
Known gaps, questions and planned work.
```

Sections without content are left out. A module README that only lists files and links is not finished.

## Code comments

### Header summaries (`///`)

Every public type, function and non-trivial data member gets a `///` summary. The first sentence says what it is or does. The following lines give the contract, as far as it is not obvious from the signature:

- units and coordinate space
- ownership and lifetime of borrowed objects
- preconditions and behavior on invalid input (ignored, clamped, or throws)
- invariants and ordering guarantees
- thread-safety, when relevant

Plain sentences are preferred. Doxygen commands (`@param`, `@return`) are allowed when a function has several non-obvious parameters.

Before:

```cpp
struct Mover {
    bool blocked{}, moving{}, junction_waiting{}, corridor_waiting{};
    float speed{2};
    std::vector<Vec2> route;
};
std::vector<Vec2> plan_route(Vec2 from, Vec2 to) const;
```

After:

```cpp
/// Autonomous transport agent that carries one product at a time between producers.
/// Owned by Simulation. Positions are in millimeters in factory coordinates.
struct Mover {
    /// Lifecycle phase; transitions are shown in the i4sim README state diagram.
    MoverState state{MoverState::parked};
    /// Maximum speed in millimeters per tick. Always positive.
    float max_speed{2};
    /// Remaining waypoints, ending at the current destination. Empty while parked.
    std::vector<Vec2> route;
};

/// Plans a route over the weighted grid from `from` to `to` (both in millimeters).
/// Returns waypoints excluding `from` and ending at `to`, or an empty route when
/// `to` is unreachable. Never throws.
std::vector<Vec2> plan_route(Vec2 from, Vec2 to) const;
```

### Implementation comments

In `.cpp` files, explain *why*: the reason behind an algorithm, an invariant being preserved, a workaround, or a tuning value. Do not narrate what the code visibly does.

```cpp
// Brake before the junction, not at it: a mover that enters at full speed cannot
// stop inside the conflict zone once another mover has been admitted.
const float braking_distance = speed * speed / max_force;
```

Larger algorithms get a short block comment above the function that names the approach and the trade-off, and links the README section that explains it in depth.

## Diagrams

Use Mermaid, which GitHub renders. Pick the form that explains the local code best:

| Question | Diagram |
| --- | --- |
| Which modules or targets depend on which? | `flowchart` component/dependency diagram |
| Which types own or use which? | `classDiagram` (composition is not inheritance) |
| Who calls whom, in what order? | `sequenceDiagram` |
| Which states exist and what moves between them? | `stateDiagram-v2` |
| How does an algorithm decide? | `flowchart` |

Every diagram reflects implemented code, labels its relationships, and is followed or preceded by one sentence that states its takeaway. Keep diagrams focused on the page's subject and link the architecture page for the big picture. No diagrams for trivial functions or one per class.

## Page layout

Every maintained Markdown page has this shape:

```markdown
# Title

One or two sentences on what this page answers.

## Table of contents

- [First section](#first-section)
- [Second section](#second-section)

[Parent guide](../README.md) · [Project overview](../README.md) · [Documentation index](README.md)

Directory guides: [child](child/README.md).
```

- The table of contents is an explicit list, because GitHub does not render `[TOC]`.
- The navigation line comes directly below the table of contents. Leave out links that would point to the page itself. The *Directory guides* line lists only direct child guides.
- No separate "Navigation" section and no link footer at the end of the page.

## Style and maintenance

- Write in English, in short paragraphs, tables and lists. Concise means no filler; it does not mean leaving out the explanation a reader needs.
- Say what is implemented, and mark anything planned as planned.
- One authoritative place per concept: explain it there and link from elsewhere. Explain locally what the local reader needs, then link for depth.
- Replace outdated text; do not append corrections below it.
- Run `python3 tools/check_docs.py` after every documentation change (in Lab: `python3 external/Kinetra/tools/check_docs.py`).
- Update docs in the same commit as the behavior they describe.

## Open points

- Doxygen HTML output from the `///` summaries is possible later; the comment style is chosen to be compatible with it.
