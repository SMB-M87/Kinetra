---
name: doc-module
description: Write or deepen the documentation of one module in Kinetra or Kinetra-Lab, consisting of a module README following the template (purpose, concepts, diagrams, how it works, invariants, how to change it, tests) and `///` contract summaries in its headers. Use when the user asks to document or explain a module ("/doc-module labs/i4sim", "documenteer deze module"), or when the feature skill finds a module README that only lists files and links.
argument-hint: "<module directory>"
---

# Document a module

Produces documentation that gives insight, following the [documentation guide](../../../docs/documentation.md). (In Kinetra-Lab, the path is `external/Kinetra/docs/documentation.md`.) Every claim must be true for the current code.

## 1. Read before writing

1. Read the existing module README and its sub-READMEs, the architecture and contract pages that mention the module, and its design documents and ADRs.
2. Read **all** headers of the module and the main `.cpp` files. For a large file (over ~500 lines), read it in sections. Build a list of public types and functions and what they do.
3. Read the tests: they show the intended behavior and which edge cases matter.
4. Write down questions you cannot answer from the code, such as *why* a constant has its value. Ask the user in one form at the end of step 1, or record them as open points; never invent a reason.

## 2. Header summaries

For every public type, function, non-trivial data member and unclear enum, add a `///` summary as described in [code comments](../../../docs/documentation.md#code-comments). Cover purpose, units and coordinate space, ownership and lifetime, preconditions, invariants and behavior on invalid input.

- Do not change code, only comments. If the code contradicts a sensible contract, note it as an open point (possible bug) instead of documenting the bug as the intent.
- Keep comments in the `.clang-format` column limit.
- In `.cpp`, add *why*-comments to non-obvious algorithms, invariants and tuning constants.

## 3. Module README

Fill in the [template](../../../docs/documentation.md#module-readme-template):

- **Concepts:** domain terms with units, the vocabulary a newcomer needs to read the code.
- **Structure:** a diagram of the real types, files or targets, plus a file table.
- **How it works:** the main loop or data flow (sequence or state diagram) and each key algorithm with its reasoning and trade-offs. This is the most important section; it is where the user learns the code.
- **Invariants and contracts:** what must always hold.
- **Changing this module:** two or three recipes for likely changes, plus pitfalls.
- **Tests:** coverage and gaps, plus the commands.
- **Open points:** questions from step 1, suspected bugs, missing tests.

Fold sub-READMEs that contain only navigation into the module README, and remove them. Update every link that pointed to them; search with `grep -rn "<old path>" --include='*.md'`.

## 4. Check and commit

1. Check every path, type name, target name and test name you mention against the code (`grep`).
2. Make sure each Mermaid diagram matches the implementation and has a takeaway sentence.
3. Run the docs check (`verify` skill, Docs level; comment-only code edits stay at Docs level).
4. Commit as `docs(<module>): ...`. If header comments and README are large, use two commits.
