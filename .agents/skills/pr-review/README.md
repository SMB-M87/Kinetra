# pr-review

A second agent reviews every pull request except a pointer-only one, or a whole stack of them. The author starts it right after opening the pull request, and you can start it too. When it finishes, the pull request has inline comments for its findings and a summary with a conclusion. The author's session has answered each finding, the two agents have settled what they could, and the pull request is either merged by its author or waiting for you with the points that need your word.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| An agent opened a pull request | Started by that agent, in a separate session | Goals, correctness and the checklist reviewed. Findings go to the author, two discussion rounds at most per point, then a merge by the author or a pull request waiting for you |
| A stack of pull requests waits | "review de stack met pr-review, vanaf de bovenste PR" | Each pull request of the stack reviewed bottom-up, each finding on the pull request it belongs to |
| You want a deeper look than the levels give | "pr-review op Kinetra#25, diepte high" | The same, with the code review at `high` even if the diff would get `low` or `medium` |

## When not to use it

- As a replacement for your decision on agent rules, skills, CI, releases or your own checks: those pull requests always wait for you ([merge rule](../../../AGENTS.md#git)).
- On a pointer-only Kinetra-Lab pull request: it needs no review ([rule](../../../AGENTS.md#git)).
- To process the comments: that is `pr-feedback`, in the session that opened the pull request.
- On a branch that is not a pull request yet: `feature` reviews its own diff with the agent's code review in its review step.

## How it works

```mermaid
flowchart LR
    A0[Author opens pull request] --> R[pr-review, separate session]
    R -->|goals, code review, checklist, own run| F[findings + conclusion]
    F --> PR[Pull request]
    PR --> A[Author: pr-feedback]
    A -->|fix or argument| R
    R -->|accepts: resolves thread| PR
    R -->|two rounds, no agreement| U[Open for you]
    PR -->|nothing open, merge rule allows| M[Author merges]
    A -->|lesson commit| L[docs/review-lessons.md]
```

*Takeaway:* the reviewer reviews and accepts; the author changes, argues and merges; anything they cannot settle, and every change to rules, CI or your own checks, stays for you.

- It reads the rules, the review lessons and the design document, and checks every goal of the roadmap item and the design document against the diff.
- It runs the agent's code review (Claude Code's `/code-review`, or a global `code-review` skill) for correctness and checks every finding itself. Then it walks a checklist from the rules: tests, verification level, docs, every copy of a changed rule or procedure, roadmap, conventions, description and merge order.
- Its depth follows the change: the reviewer's session runs on the most capable model at high reasoning effort, one level higher for a large pull request with much logic, and the code review at `low` for docs, `medium` for code in one module, `high` for public headers, simulation behavior, CMake, CI and releases ([levels](SKILL.md#levels)). It names no model or product, so it works with any agent.
- It reads CI's results, graphics tests included, and adds what CI does not cover: in Lab, a screenshot of the changed scene.
- Everything it posts on GitHub, the conclusion included, is in English and starts with **Agent review**. It reports to you in the chat in Dutch.
- It resolves the threads it accepts, and never approves, requests changes, pushes or merges.
- A finding that shows a kind of mistake that can recur becomes a proposed lesson. A lesson that keeps coming back is proposed to you as a rule.

## Related skills

- `pr-feedback` answers the findings in the author's session, adds the lessons, and leads to the merge under the merge rule.
- `feature` starts this review after opening a pull request, and reads the lessons page before building.
- The agent's code review (Claude Code's `/code-review`, or a global `code-review` skill, as in Codex) does the correctness pass.
