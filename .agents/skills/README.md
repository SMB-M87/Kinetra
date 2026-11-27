# Agent skills

Procedures that agents (Claude Code, Codex) load on demand. This page is the overview of all repository skills. Codex reads `.agents/skills/` and Claude Code reads `.claude/skills/`, which is a symlink to this directory. Kinetra-Lab links to the same directory, so both repositories use one set of skills.

Choose a skill below to see example requests and expected results. Each guide links to the agent's procedure in its sibling `SKILL.md`, which follows the [Agent Skills format](https://agentskills.io). [Skill README template](../../docs/documentation.md#skill-readme-template)

## Table of contents

- [Skills](#skills)
- [How they fit the cycle](#how-they-fit-the-cycle)
- [Maintenance](#maintenance)

[Agent rules](../../AGENTS.md) · [Project overview](../../README.md) · [Documentation index](../../docs/README.md)

## Skills

| Skill | Use | Invoked |
| --- | --- | --- |
| [feature](feature/README.md) | Run a change through the full development cycle, up to an open pull request or a stack of them | At the start of every change, or `/feature` |
| [grilling](grilling/README.md) | Settle open decisions with the user in rounds of forms | On request, or from `feature` |
| [verify](verify/README.md) | Choose checks by impact and confirm pull request CI for full verification | After changes, before committing |
| [doc-module](doc-module/README.md) | Write a module README and `///` header summaries | On request, or from `feature` |
| [refactor](refactor/README.md) | Behavior-preserving restructuring in tested steps | On request, or scheduled by a design document |
| [adr](adr/README.md) | Record an architecture decision | When a decision qualifies |
| [pr-feedback](pr-feedback/README.md) | Process feedback already posted on GitHub | In the author’s session once there is feedback to handle |
| [pr-review](pr-review/README.md) | Check an open pull request or stack independently | On the user’s request |
| [roadmap](roadmap/README.md) | Keep the backlog in `docs/roadmap.md`: ideas from a brainstorm, items to pick up, items closed on merge | On request (`/roadmap`, a brainstorm), or from `feature` and `grilling` |

The user's global skill set (book references such as refactoring, clean architecture and a philosophy of software design, plus `teach`, `grill-me` and `code-review`) is installed per machine and is not copied here.

## How they fit the cycle

```mermaid
flowchart LR
    F[feature] --> G[grilling]
    F --> A[adr]
    F --> R[refactor]
    F --> V[verify]
    F --> D[doc-module]
    R --> V
    D --> V
    F -->|GitHub feedback| P[pr-feedback]
    P --> V
    U[User requests an independent review] --> PR[pr-review]
    PR -->|findings| P
    P -->|answers| PR
    F --> RM[roadmap]
    G --> RM
    RM -->|item picked up| F
```

`feature` drives the [workflow](../../docs/workflow.md), including test design, verification and the author’s final review. `roadmap` supplies agreed work and records follow-ups. Use `pr-review` on request and `pr-feedback` for feedback on GitHub.

## Maintenance

- Skills follow the rules in [AGENTS.md](../../AGENTS.md) and link to the docs pages instead of copying them, so a rule changes in one place.
- Skill names must not collide with the global skills or with Claude Code built-ins (`code-review`, `simplify`, `init`, …).
- Skills stay agent-neutral, so Claude Code, Codex and other agents can follow them: they name no model, and a step that uses one agent's command or tool names it as an example next to a description of the action, or gives the equivalent for the others.
- A skill that reaches outside the repository (push, PR, network) needs an explicit rule in AGENTS.md first.
- A change that adds or changes a skill updates its `README.md` and the table on this page in the same commit.
