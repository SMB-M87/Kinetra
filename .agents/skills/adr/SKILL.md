---
name: adr
description: Record an architecture decision record (ADR) in Kinetra or Kinetra-Lab when a decision is hard to reverse, surprising without context and the result of a real trade-off. Use when the user asks for an ADR ("/adr", "leg dit besluit vast"), or when the feature or grilling skill settles such a decision.
argument-hint: "<decision title>"
---

# Architecture decision record

## 1. Does it qualify?

All three must be true:

1. **Hard to reverse:** changing your mind later costs real work (public API, target boundary, file format, ownership model).
2. **Surprising without context:** a future reader would ask "why on earth is it done this way?".
3. **A real trade-off:** there were genuine alternatives, and one was chosen for specific reasons.

If any is false, the design document's **Decisions** table is enough. Say so and stop.

## 2. Where

| Decision about | Location | Index |
| --- | --- | --- |
| Engine boundaries, public API, `Kinetra::` targets | Kinetra `docs/decisions/` | Table in Kinetra [docs/decisions/README.md](../../../docs/decisions/README.md#decision-index) |
| Lab application structure, Lab-only boundaries | Lab `docs/decisions/`, created with its own README when the first Lab ADR is written | Lab `docs/README.md` |

Numbering continues from the highest existing number in that directory: `NNNN-short-slug.md`.

## 3. Format

Follow the existing Kinetra ADRs (for example [0012](../../../docs/decisions/0012-traffic-admission.md)). (In Kinetra-Lab, the path is `external/Kinetra/docs/decisions/`.)

```markdown
# NNNN: <Decision title>

Status: proposed | accepted | superseded by [NNNN](NNNN-slug.md). <One-sentence scope.>

## Table of contents
- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context
The forces at play and the problem, with evidence.

## Decision
What was chosen. Name the rejected alternatives in one line each, with why.

## Consequences
What becomes easier or harder; follow-up work; tests that guard the decision.
```

Keep it short: an ADR records *that* and *why*, not the full design (which lives in the design document).

## 4. Maintain

- Add the ADR to the index table. When it replaces or amends an older decision, set the old ADR's status to `superseded by [NNNN](...)` or `amended by [NNNN](...)` and update its row in the index; never delete history.
- Link the ADR from the design document, and from the architecture or contract page it affects.
- Run the docs check, then commit as `docs(adr): record NNNN <slug>` (or together with the change it belongs to).
