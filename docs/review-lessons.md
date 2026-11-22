# Review lessons

Mistakes that reviews and CI found in Kinetra work and that can happen again, each with the reason and what to do instead. Agents read this page before they build a change (`feature` skill) and before they review one (`pr-review` skill), so they do not repeat each other's mistakes. Kinetra-Lab keeps its own page for Lab-specific lessons and reads this one too.

## Table of contents

- [Format](#format)
- [Docs and links](#docs-and-links)
- [CI and tooling](#ci-and-tooling)
- [Git and stacks](#git-and-stacks)
- [Code](#code)

[Documentation index](README.md) · [Agent rules](../AGENTS.md) · [Workflow](workflow.md)

## Format

One list item per lesson, under the topic it belongs to:

```markdown
- **<What to do, as an imperative>** · added YYYY-MM-DD · source: <pull request>
  <Why: the mistake it prevents, in one or two sentences.>
```

- A lesson is a pattern: a kind of mistake that can recur. A one-off slip is not a lesson.
- The author adds it in a commit on the pull request where it was found (`docs(lessons): <lesson>`), so the lesson merges with the fix.
- A lesson that keeps coming back is proposed to the user as a rule. Once the user agrees, it moves to [AGENTS.md](../AGENTS.md) or the [conventions](conventions.md) and leaves this page.
- Replace a lesson that turns out wrong instead of adding a correction below it.

## Docs and links

- **On a stacked branch, link only to files that exist on that branch** · added 2026-11-21 · source: #20
  Every pull request of a stack runs the docs check on its own tree; a link to a design document that only the branch above adds fails it. Name such a document in plain text, or link it from the branch above.

- **Scope a new ban to the case it targets, and check it against every procedure that still performs that action** · added 2026-11-22 · source: #27
  "Never push a pointer move onto an open pull request" was meant for the move to the merge commit, but read literally it also forbade the branch-pointer updates that the stack rules still need. Name the exact case in the rule, then search the rules for every step that does the forbidden thing.

- **Search for copies of a changed step case-insensitively, on its key words, not on one copy's sentence** · added 2026-11-22 · source: #30
  The search for "After opening it" matched only the sentence in `AGENTS.md`; the skills overview and the `pr-feedback` guide said "after opening a pull request" and "after every pull request". Use `grep -rni` with each key word of the step in lower case.

## CI and tooling

- **Never write a host name, URL or version into configuration from memory** · added 2026-11-21 · source: #22
  The first commit of #22 listed three mirrors of the Arch Linux Archive that do not exist, and only CI showed it. Check the value with a request first; when the session cannot reach it, say so and prefer a source that names no host, such as the image's own mirror list.

## Git and stacks

No lessons yet.

## Code

- **After a layout change, assert the content whose position must change** · added 2026-11-22 · source: #31
  A fixed close button can keep responding even when scroll offsets or a neighbouring overlay are wrong. Check which rows, bars or captions appear at the new positions.
