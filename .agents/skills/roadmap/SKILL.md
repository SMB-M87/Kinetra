---
name: roadmap
description: Keep the backlog in docs/roadmap.md of Kinetra or Kinetra-Lab up to date. Add ideas from a brainstorm, shape an idea into an item that can be picked up, pick the next item, or close an item when its pull request merges. Use when the user brainstorms, shares ideas or wishes, or says /roadmap, "zet dit op de roadmap", "wat staat er nog open", "wat pakken we hierna op". The feature and grilling skills call it for follow-up work.
argument-hint: "<idea, item or question about the backlog>"
---

# Roadmap: the backlog of planned work

Each repository has one backlog in `docs/roadmap.md`. From Kinetra-Lab, Lab's is `docs/roadmap.md` and Kinetra's is `external/Kinetra/docs/roadmap.md`. The sections, the item format and when an item moves are defined in the [documentation guide](../../../docs/documentation.md#roadmap); read that section first. Talk to the user in Dutch; write the roadmap in English.

## 0. Find the right roadmap

- Engine work (simulation, runtime, launcher library, rendering, platforms) goes in Kinetra's roadmap. Applications, Lab tools, media and the Lab build go in Lab's.
- Work that needs both gets an item in each, each with its own part and a link to the other.
- Read the whole roadmap before editing, so you merge with an existing item instead of adding a duplicate.

## 1. Brainstorm: collect ideas

When the user shares ideas or thinks aloud:

1. Ask only what you need to write a one-line item: the goal, and why it matters. Do not design yet.
2. Look up the facts the idea rests on (code, docs, other repositories) and add them as one or two sentences, with paths. Mark guesses as questions.
3. Add each idea under **Ideas** with today's date. Keep the user's intent in their words, translated to English.
4. Show the user the items you added, in a short list in Dutch, and ask whether the order is right.

## 2. Shape: move an item up

An item moves from Ideas or Later to **Next** when the goal is clear, a first step is concrete and nothing blocks it.

- Settle open questions with the user; with several load-bearing ones, use the `grilling` skill.
- Large items are split: the first slice goes to Next, the rest stays under Later with `Depends on`.
- A decision that meets the ADR criteria is noted as a question on the item; the ADR itself is written on the branch that implements it.

## 3. Pick up: start an item

When the user asks what is next, propose the first item of **Next** (and say why, if you suggest another). Once the user agrees, start it with the `feature` skill. On the new branch, move the item to **Now** with its branch and design document, in the branch's first commit.

## 4. Close: remove a finished item

On the branch that finishes an item, in the same commit as the design document's status `implemented`, or in the last commit of a docs-only branch, which has no design document and whose pull request is the record:

- Remove the item from **Now**. Its design document is the record; do not keep a done list.
- Add the follow-ups from the design document's *Open points* that someone intends to act on, as items in **Next** or **Later**, linking the open point.
- When the work spans both repositories, update the other repository's item as well, on that repository's branch.

## Rules

- Never delete an idea of the user without asking; move it to **Later** or ask whether it is dropped.
- Replace outdated item text; do not append "update:" lines.
- Edit the roadmap on a branch, like every other file, and run the docs check after editing.
- Within each section the order is the priority. Reorder only on the user's word, or note your proposal in the hand-over.
