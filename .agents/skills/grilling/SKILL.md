---
name: grilling
description: Interview the user in rounds of multiple-choice forms until every load-bearing decision of a plan or design is settled, and record the outcome in the design document, or for a docs-only change in the pull request description. Use when the user asks to grill, challenge or stress-test a plan, or when the feature skill reaches open load-bearing decisions. Not for routine work, lookups or decisions already made in the conversation.
---

<!-- Based on mattpocock/skills "grilling" (MIT, (c) 2026 Matt Pocock); assumptions ledger and load-bearing/cosmetic tiers after chaseai-yt "crucible" (MIT). Adapted for Kinetra: forms per round, outcome recorded in docs/design/. -->

# Grilling

Reach a shared understanding with the user before building. Map the work as a **design tree**: every decision branches into the decisions that depend on it. Work the tree in **rounds**.

## 0. Recon and assumptions ledger

Finding facts is your job; deciding is the user's. Before asking anything, read the code, docs and git state, and run cheap commands. For broad questions, dispatch a sub-agent with a narrow brief and keep asking the questions that do not depend on it.

Post **one list** in the chat of everything you resolved yourself, with a source on each line:

```markdown
**Assumptions** (correct them in one reply; anything you don't comment on counts as confirmed)
1. <assumption> — source: <path:line / command output / doc>
```

If the task has a design document, write the draft there now, guesses marked as guesses. Otherwise keep a short plan in the session scratchpad.

## 1. Split load-bearing from cosmetic

- **Load-bearing:** a wrong guess costs a rewrite, a migration, a broken contract or the user's trust. These go into forms.
- **Cosmetic:** renameable or reversible later. List these in plain text, each with your recommendation; silence means acceptance.

The test is the *what breaks if we guess wrong* line. If it comes out vague, the decision is cosmetic.

## 2. Ask each round as a form

The **frontier** is every decision whose prerequisites are settled. Ask the whole frontier in one round. A question whose answer depends on another open question in this round waits for the next round.

Use the interactive question tool: `AskUserQuestion` in Claude Code, `request_user_input` in Codex. Never put a round in plain text. If no such tool is available, say so and pause instead of emulating it.

Per question:

- `header`: number plus short title (max 12 characters), for example `Q3 Tests`.
- `question`: complete and self-contained, ending with a question mark. The user may see only the dialog, so name the fact it rests on.
- `options`: 2–4 concrete, mutually exclusive choices. Put your recommendation first, with its label ending in "(Aanbevolen)" when the conversation is in Dutch, otherwise "(Recommended)". Each `description` states the consequence (what becomes true, what it costs), not the option restated.
- Never add an "Other" option; the tool adds a free-text answer.
- `multiSelect: true` only when the choices are not mutually exclusive.

One form holds at most 4 questions; a larger frontier takes several forms in a row, most fundamental first. A short text before the first form with the findings the questions rest on is fine.

## 3. Between rounds

Read the answers carefully: free-text answers may change scope or add requirements. When a round moved something (an answer that differs from your recommendation, a correction, a new branch), post a short summary:

- **Understood:** the facts now settled.
- **Assumptions:** each marked *verified* or *guess*.
- **Risks → questions:** each risk turned into a concrete question for the next round.

Recompute the frontier and ask the next round. When every answer matched your recommendation, say so in one line; that is a signal, not proof.

## 4. Finish

- Stop when the frontier is empty, or immediately when the user says stop ("stop", "klaar", "doe maar"). Unresolved branches then become stated assumptions.
- When the scope is too big for one change, propose splitting it into phases and grill only the first phase in detail. The later phases, and branches of the tree the user deferred, become roadmap items (`roadmap` skill), so they are not lost.
- Record every decision in the design document's **Decisions** table, or for a docs-only change in the pull request description: decision, choice, why. Note which choices differed from the recommendation.
- Ask one final form to confirm the shared understanding, then start the work. Irreversible or outward-facing actions keep their own confirmation rules.

## Do not grill

Reading, explaining, status checks and one-line lookups. Never re-ask a decision the user already made in this conversation.
