# Design documents

Every branch has a design document, written before the code. It records the problem, the decisions taken with the user, the test plan, the review outcome and the text for the pull request. Design documents are kept after merging: together they are the change history *with reasons*.

## Table of contents

- [Lifecycle](#lifecycle)
- [Template](#template)
- [Index](#index)

[Parent guide](../README.md) · [Project overview](../../README.md) · [Documentation index](../README.md) · [Workflow](../workflow.md)

## Lifecycle

```mermaid
stateDiagram-v2
    [*] --> draft: branch created
    draft --> accepted: open decisions settled
    accepted --> implemented: verified, reviewed, documented
    draft --> abandoned: not pursued (reason recorded)
    accepted --> abandoned
    implemented --> [*]
```

- File name: `YYYY-MM-DD-<topic>.md`, with the date the branch starts.
- A design document describes one change. Durable decisions also become an ADR in [decisions](../decisions/README.md), and the design document links to it.
- Once implemented, a design document is history. Current behavior is described by READMEs, contracts and `///` comments, not by old design documents.

## Template

```markdown
# <Title>

Status: draft | accepted | implemented | abandoned · Branch: `<type>/<topic>` · Related: <links>

<One paragraph: what changes and why now.>

## Table of contents
<local heading links>

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem
What is wrong or missing, with evidence (paths, measurements).

## Goals and non-goals
What this change achieves; what it deliberately leaves alone.

## Decisions
| Decision | Choice | Why |
Including the decisions settled with the user.

## Design
How it will work. A diagram if structure or interaction changes.

## Affected areas
Files, targets, public API, docs.

## Test plan
Which behavior is proven by which test; the verification level.

## Review
Findings and how they were resolved.

## Verification
Commands and results.

## Pull request
Title and description used for `gh pr create`: Summary, Changes, Design,
Verification, Review notes, Dependencies and merge order, Follow-up.

## Open points
```

## Index

| Date | Document | Status |
| --- | --- | --- |
| 2026-11-18 | [Development process and agent rules](2026-11-18-development-process.md) | implemented |
| 2026-11-18 | [Settings overlay, glyphs and session navigation](2026-11-18-overlay-glyphs-navigation.md): Lab roadmap phase 3, engine side | accepted |
