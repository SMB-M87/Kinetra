---
name: refactor
description: Restructure C++ code in Kinetra or Kinetra-Lab without changing behavior, in small named steps (after Fowler's Refactoring and Feathers' Working Effectively with Legacy Code). Name the smells, present a plan, build a test safety net first, then build and test after every step. Use only when the user explicitly asks (/refactor, "refactor dit", "ruim deze klasse op") or when a design document schedules a refactor.
argument-hint: "[file or directory; empty = changes on this branch]"
---

# Refactor

Improve structure **without changing behavior**. Refactoring and behavior change never share a step or a commit. A bug found on the way is recorded and reported, not silently fixed. For a quick tidy-up of a diff, use `/simplify`; this skill is for structural problems that need a plan.

## 1. Scope

- With an argument: that file or directory.
- Without an argument: the branch changes (`git diff --stat main...HEAD`, `git status --short`). Read only the relevant files, never the whole diff at once.
- If the same files contain uncommitted work from the user, say so before starting.
- Read the [conventions](../../../docs/conventions.md) for the topics you touch; the result must meet them. (In Kinetra-Lab, the path is `external/Kinetra/docs/conventions.md`.)

## 2. Name the smells

For each problem, give the smell, the location (`path:line`) and why it hurts *here*. Leave out matters of taste.

| Smell | Sign in this codebase | Usual refactoring |
| --- | --- | --- |
| Long function | Sections with comment headers, more than ~60 lines | Extract Function |
| Large class / divergent change | Members used by only some methods; changes for unrelated reasons | Extract Class by responsibility |
| Flag soup / hidden state machine | Several `bool`s describing one lifecycle; nested ternaries decoding them | Replace flags with `enum class` state (State pattern if behavior differs per state) |
| Flag argument | `bool` parameter that switches behavior | Separate functions or `enum class` parameter |
| Long parameter list | More than four parameters that travel together | Introduce Parameter Object |
| Duplicated code | Same block in several sessions or labs | Extract Function into a shared helper; reusable mechanism → Kinetra |
| Magic number | Same literal or expression repeated | Named `constexpr` / extracted function |
| Global mutable state | `static` arrays, singletons, `std::rand` | Move into an owning instance; inject a seeded RNG |
| Feature envy | Function mostly reading another object's data | Move Function |
| Public mutable data with invariants | Fields flipped from outside to get past a guard | Encapsulate Field, add an operation |
| Layer inversion | Logic including presentation or app headers | Move the dependency outward; introduce an interface at the seam |
| Dead code | Unused fields, commands never set, unreachable paths | Remove (after checking tests and other repositories) |

## 3. Safety net

Refactoring without tests is guessing. Per area:

- **Tests cover the behavior:** run them, and they must pass before the first step.
- **Behavior not covered:** write **characterization tests** first. They capture what the code does now, even if it looks odd; for simulations, use a fixed seed with a golden summary or observable invariants. Commit them separately (`test(<scope>): characterize ...`).
- **No practical way to test** (for example, rendering without a context): limit yourself to compiler-checked mechanical refactorings such as rename, Extract or Inline Function, Move to a private helper and removing provably dead code. Name the **seam** that would make it testable later.

## 4. Contracts that must not break

Check before renaming or moving anything:

- Public `include/kinetra/` headers and `Kinetra::` targets: Lab and the external consumer depend on them. A change there is not a pure refactor; it needs a design decision and possibly an ADR.
- File formats: i4sim JSON blueprints and their fields.
- Units, timing (fixed ticks) and seeded determinism.
- CLI arguments and launcher behavior documented in READMEs.

## 5. Present the plan

Before changing anything, show the smells by priority, the named refactorings in execution order, the safety net, and the contracts affected. Wait for agreement. In the `feature` flow, this plan goes into the design document.

## 6. Execute in small steps

Per step:

1. Exactly one refactoring.
2. Build and test at the targeted level (`verify` skill).
3. If the result is red or does not build, revert the step and make it smaller. Do not build further on a broken step.
4. Commit as `refactor(<scope>): <named refactoring>`.

## 7. Hand over

- The refactorings performed, per file, with their commits.
- Build and test result per step, and which steps relied only on the compiler.
- What you deliberately skipped, and why (contract, no safety net, taste).
- Bugs or suspicious behavior found, separate from the refactor.

## Background

The approach paraphrases Martin Fowler, *Refactoring* (smell catalogue, named refactorings, small steps, refactoring separate from behavior) and Michael Feathers, *Working Effectively with Legacy Code* (characterization tests, seams, safety net first). When the global book skills `refactoring` or `working-effectively-with-legacy-code` are available, consult them for deeper guidance.
