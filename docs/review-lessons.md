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

- **State another repository's procedure by linking to it, not by restating its conditions** · added 2026-11-23 · source: #37
  Kinetra's copies of Lab's CI skip left out the three Lab paths that always build; a link stays right when the other repository changes.

- **Put an exemption in every skill a calling skill hands work to, not only in the caller** · added 2026-11-23 · source: #37
  `feature` sent a docs-only change's decisions to the pull request description, but `grilling` and `adr`, which also run on their own, still wrote into a design document.

## CI and tooling

- **Never write a host name, URL or version into configuration from memory** · added 2026-11-21 · source: #22
  The first commit of #22 listed three mirrors of the Arch Linux Archive that do not exist, and only CI showed it. Check the value with a request first; when the session cannot reach it, say so and prefer a source that names no host, such as the image's own mirror list.

- **Make a job that claims to test a distribution's packages use them, not the runner's preinstalled tools** · added 2026-11-22 · source: #33
  GitHub's runner images put newer CMake, compilers and libraries ahead of what `apt` installs, so #33's Ubuntu job first passed on CMake 3.31 while the documented command installs 3.28. Put the distribution's paths first and print the path and version of each tool the docs name.

- **Gate an aggregating required job with `always()`, never `!cancelled()` or `success()`, and test the result of every job it needs** · added 2026-11-23 · source: #35
  GitHub reports a skipped job as passed, even when it is a required check, so a gate that is skipped on cancellation or failure lets the pull request merge untested. #35's first `checks` used `!cancelled()`.

- **Under an explicit `shell: bash`, never pipe a command into `head`** · added 2026-11-23 · source: #35
  An explicit `shell: bash` runs every step with `-o pipefail`, so when `head` closes the pipe early the writer dies of SIGPIPE and the step fails with exit code 141, at random. #35's first run failed both matrix entries on `ldd --version | head -1`. Use `sed -n 1p`, which reads all of its input.

- **Make a CI skip decision fail closed: skip only on an explicit "nothing left", never on a non-zero pipeline status** · added 2026-11-23 · source: #37
  `if ! a | b | grep -q .` reads any failure, such as a SIGPIPE under `pipefail`, as "no source changed" and skips the build; test for `grep`'s status 1 instead.

- **Do not let a pull request's own copy of a skip filter decide its own CI unchecked** · added 2026-11-23 · source: #37
  On `pull_request`, the workflow runs from the pull request, so one dropped `^` would have skipped its build and every later one. Let the base branch's copy decide, as Kinetra-Lab#83 does with its tip, or check the filter and the decision against known source paths and build in full when they fail.

## Git and stacks

No lessons yet.

## Code

- **After a layout change, assert the content whose position must change** · added 2026-11-22 · source: #31
  A fixed close button can keep responding even when scroll offsets or a neighbouring overlay are wrong. Check which rows, bars or captions appear at the new positions.

- **State the floating-point flags a determinism helper needs, and check the generated code under them** · added 2026-11-22 · source: #32
  `kinetra::asin` exists to avoid `asinf`, yet with `-ffast-math` GCC and Clang emit `asinf` again; an inline header function compiles with each consumer's flags. Say in the contract which flags break it, and look at the assembly under them.

- **Pass every input of a C-library test through a run-time value** · added 2026-11-22 · source: #32
  A compiler folds a call with constant arguments, and its folding is correctly rounded, so such a test checks the compiler, not the library. The first version of #32's rounding case passed with `std::` for exactly this reason.
