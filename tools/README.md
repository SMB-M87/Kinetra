# Tools

Development scripts shared by Kinetra and Kinetra-Lab. They need only Python 3 and Git: no build, no network.

## Table of contents

- [check_docs.py](#check_docspy)
- [check_copies.py](#check_copiespy)
- [make_icon.py](#make_iconpy)
- [Open points](#open-points)

[Parent guide](../README.md) · [Documentation index](../docs/README.md)

## check_docs.py

The **Docs** [verification level](../docs/testing.md#verification-levels). It checks every tracked or untracked-but-not-ignored Markdown file in the repository for:

- relative links whose target file or directory does not exist;
- `#anchor` fragments that match no heading in the target page, using GitHub's slug rules (including `-1`, `-2` for duplicate headings).

Fenced code blocks and inline code are skipped, so templates and examples may contain placeholder links. External URLs are not fetched. Submodule contents are not scanned, but links into a checked-out submodule are resolved.

```sh
python3 tools/check_docs.py                    # in Kinetra
python3 external/Kinetra/tools/check_docs.py   # in Kinetra-Lab (any path inside the repository works)
```

Output is one `path:line: problem` per broken link, then a summary. The exit status is 1 when anything is broken, so the same command can run in CI.

## check_copies.py

The mechanical part of the rule that every copy of a changed rule, procedure or term changes with it ([AGENTS.md](../AGENTS.md#repository-documentation)); the `verify` skill runs it at every level. It compares the working tree with the merge base of `--base` (default `origin/main`, else `main`) and collects what the change removes from a file:

| Kind | Removed when |
| --- | --- |
| CI job | A job id leaves a workflow's `jobs:` in `.github/workflows/` |
| CMake option | `option(NAME` leaves a `CMakeLists.txt` or `*.cmake` |
| Preset | A `"name"` leaves `CMakePresets.json` |
| Flag | `-DNAME` leaves a changed file |
| Markdown name | An inline code span leaves a changed `*.md` file; a plain lowercase word such as `main` is skipped |

It then lists every place in a tracked or untracked-but-not-ignored `*.md`, `*.yml` or `*.yaml` file, submodules included, that still names one of them: a job as "`x` job", "job `x`", "CI / x" or in `needs:`, a preset as `--preset x` or "`x` preset", the others by name. Each `--term` adds an old wording, searched case-insensitively. Dated design documents (`docs/design/20*`) and review lessons record their time and are skipped unless `--all` is given.

```sh
python3 tools/check_copies.py --term 'warnings reported'          # in Kinetra
python3 external/Kinetra/tools/check_copies.py --base origin/main  # in Kinetra-Lab
```

A hit is a candidate: the name may still be right there, so look at each one. The exit status is 0, or 1 with `--strict` when anything was found, and 2 on a Git or usage error. On the history it was written for, it finds the copy that #35's review found (testing still named "job `ubuntu`" after the job became a matrix run) and, with `--term 'skipped on Windows'`, the one Kinetra-Lab#78's first review found.

## make_icon.py

Draws the Kinetra application icon from a few vector shapes and writes the three files derived from it. The shapes and colors are constants at the top of the script, so changing the icon means changing those and regenerating.

| Output | Used by |
| --- | --- |
| `resources/kinetra.ico` (16 to 256 px) | The Windows executable resource, through `kinetra_executable_icon(<target>)` in [KinetraIcon.cmake](../cmake/KinetraIcon.cmake) |
| `resources/kinetra-icon.png` (256 px) | Documentation and desktop files |
| `src/runtime/window_icon.inc` (64 px RGBA) | `Runtime`, which sets it as the window icon |

```sh
python3 tools/make_icon.py          # regenerate
python3 tools/make_icon.py --check  # exit 1 when a committed file no longer matches the script
```

The check compares decoded pixels rather than bytes, because another zlib version may compress the same PNG differently. CI runs it next to the documentation check.

## Open points

- `check_copies.py` finds names and given wordings, not a rule restated in other words; the review still reads for those.
- A check that every module directory has a README with content would enforce the [documentation rule](../docs/documentation.md#what-goes-where); it is not automated yet.
