# Tools

Development scripts shared by Kinetra and Kinetra-Lab. They need only Python 3 and Git: no build, no network.

## Table of contents

- [check_docs.py](#check_docspy)
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

## make_icon.py

Draws the Kinetra application icon from a few vector shapes and writes the three files derived from it. The shapes and colors are constants at the top of the script, so changing the icon means changing those and regenerating.

| Output | Used by |
| --- | --- |
| `resources/kinetra.ico` (16 to 256 px) | The Windows executable resource, through `kinetra_executable_icon(<target>)` in [KinetraIcon.cmake](../cmake/KinetraIcon.cmake) |
| `resources/kinetra-icon.png` (256 px) | Documentation and desktop files |
| `src/engine/window_icon.inc` (64 px RGBA) | `Runtime`, which sets it as the window icon |

```sh
python3 tools/make_icon.py          # regenerate
python3 tools/make_icon.py --check  # exit 1 when a committed file no longer matches the script
```

The check compares decoded pixels rather than bytes, because another zlib version may compress the same PNG differently. CI runs it next to the documentation check.

## Open points

- A check that every module directory has a README with content would enforce the [documentation rule](../docs/documentation.md#what-goes-where); it is not automated yet.
