#!/usr/bin/env python3
"""Find the copies of names that a change removes.

Usage: python3 tools/check_copies.py [--base REF] [--term TEXT ...] [--all] [--strict]

Compares the working tree with the merge base of REF (default: origin/main, else main)
and collects the names the change removes from a file:

- CI jobs: job ids that leave a workflow's ``jobs:`` (``.github/workflows/*.yml``);
- CMake options: ``option(NAME`` that leaves a ``CMakeLists.txt`` or ``*.cmake``;
- presets: ``"name": "..."`` entries that leave ``CMakePresets.json``;
- flags: ``-DNAME`` that leaves a changed file;
- Markdown names: inline code spans (``like this``) that leave a changed ``*.md`` file,
  except a plain lowercase word such as ``main``, which names too many things to search for.

It then searches every tracked or untracked-but-not-ignored ``*.md``, ``*.yml`` and
``*.yaml`` file, submodules included, for places that still name them, and prints one
``path:line: text`` per hit. ``--term`` adds old wordings to search for, case-insensitively.
Dated design documents (``docs/design/20*``) and review lessons are records of their time
and are skipped unless ``--all`` is given.

A hit is a candidate, not an error: a name can still be right where it is. Look at each one.
Exit status: 0, or 1 with ``--strict`` when there is a hit (2 on a usage or Git error).
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

SEARCHED = ("*.md", "*.yml", "*.yaml")
JOB = re.compile(r"^  ([A-Za-z0-9_-]+):\s*(?:#.*)?$")
TOP_LEVEL = re.compile(r"^[A-Za-z_][^:]*:")
OPTION = re.compile(r"\boption\(\s*([A-Za-z_][A-Za-z0-9_]*)")
PRESET = re.compile(r'"name"\s*:\s*"([^"]+)"')
FLAG = re.compile(r"-D([A-Z][A-Z0-9_]{2,})")
CODE_SPAN = re.compile(r"(?<!`)`([^`\n]{2,80})`(?!`)")
FENCE = re.compile(r"^ {0,3}(`{3,}|~{3,})")
PLAIN_WORD = re.compile(r"[a-z]+")
SHOWN = 8  # hits listed per name; the rest are counted


@dataclass
class Removed:
    """One removed name: what it is, where it left, and how its copies read."""

    kind: str
    name: str
    sources: set[str] = field(default_factory=set)
    patterns: list[re.Pattern[str]] = field(default_factory=list)


def git(root: Path, *args: str, check: bool = True) -> str:
    # errors="replace": an undecodable byte, as in a binary file or a text file that is not
    # UTF-8, becomes a replacement character instead of stopping the check.
    result = subprocess.run(
        ["git", "-C", str(root), *args], capture_output=True, text=True, encoding="utf-8", errors="replace"
    )
    if check and result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)}: {result.stderr.strip()}")
    return result.stdout if result.returncode == 0 else ""


def merge_base(root: Path, ref: str | None) -> str:
    candidates = [ref] if ref else ["origin/main", "main"]
    for candidate in candidates:
        base = git(root, "merge-base", candidate, "HEAD", check=False).strip()
        if base:
            return base
    raise RuntimeError(f"no merge base with {' or '.join(candidates)}")


def old_text(root: Path, base: str, path: str) -> str:
    return git(root, "show", f"{base}:{path}", check=False)


def new_text(root: Path, path: str) -> str:
    file = root / path
    return file.read_text(encoding="utf-8", errors="replace") if file.is_file() else ""


def job_ids(text: str) -> set[str]:
    """Job ids directly under a workflow's top-level ``jobs:`` key."""
    ids, inside = set(), False
    for line in text.splitlines():
        if TOP_LEVEL.match(line):
            inside = line.startswith("jobs:")
        elif inside:
            match = JOB.match(line)
            if match:
                ids.add(match.group(1))
    return ids


def code_spans(text: str) -> set[str]:
    """Inline code spans outside fenced blocks."""
    spans, fence = set(), None
    for line in text.splitlines():
        match = FENCE.match(line)
        if match:
            marker = match.group(1)
            fence = marker[0] if fence is None else (None if marker.startswith(fence) else fence)
            continue
        if fence is None:
            spans.update(span.strip() for span in CODE_SPAN.findall(line) if span.strip())
    return spans


def job_patterns(job: str) -> list[re.Pattern[str]]:
    name = re.escape(job)
    return [
        re.compile(rf"`{name}`\s+job", re.I),
        re.compile(rf"\bjob\s+(?:id\s+)?[`\"]{name}[`\"]", re.I),
        re.compile(rf"\"{name}\"\s+job", re.I),
        re.compile(rf"\bCI\s*/\s*{name}\b", re.I),
        re.compile(rf"^\s*needs:.*\b{name}\b"),
    ]


def preset_patterns(preset: str) -> list[re.Pattern[str]]:
    name = re.escape(preset)
    return [
        re.compile(rf"--preset[ =]{name}(?![\w-])"),
        re.compile(rf"`{name}`\s+(?:test\s+|workflow\s+|configure\s+)?preset", re.I),
        re.compile(rf"preset\s+`{name}`", re.I),
    ]


def word(name: str) -> list[re.Pattern[str]]:
    return [re.compile(rf"(?<![\w-]){re.escape(name)}(?![\w-])")]


def collect(root: Path, base: str) -> dict[tuple[str, str], Removed]:
    """Every name some changed file had at the merge base and no longer has."""
    removed: dict[tuple[str, str], Removed] = {}

    def add(kind: str, name: str, source: str, patterns: list[re.Pattern[str]]) -> None:
        entry = removed.setdefault((kind, name), Removed(kind, name, patterns=patterns))
        entry.sources.add(source)

    changed = git(root, "diff", "--name-only", "--no-renames", base).splitlines()
    for path in changed:
        before, after = old_text(root, base, path), new_text(root, path)
        # A NUL byte marks a binary file, such as a deleted video: its decoded bytes would
        # only produce chance matches, like a "-D" flag in a run of random characters.
        if not before or "\0" in before:
            continue
        name = Path(path).name
        if path.startswith(".github/workflows/") and name.endswith((".yml", ".yaml")):
            for job in job_ids(before) - job_ids(after):
                add("CI job", job, path, job_patterns(job))
        if name == "CMakeLists.txt" or name.endswith(".cmake"):
            for option in set(OPTION.findall(before)) - set(OPTION.findall(after)):
                add("CMake option", option, path, word(option))
        if name == "CMakePresets.json":
            for preset in set(PRESET.findall(before)) - set(PRESET.findall(after)):
                add("preset", preset, path, preset_patterns(preset))
        for flag in set(FLAG.findall(before)) - set(FLAG.findall(after)):
            add("flag", f"-D{flag}", path, word(flag))
        if name.endswith(".md"):
            for span in code_spans(before) - code_spans(after):
                if PLAIN_WORD.fullmatch(span):
                    continue
                add("Markdown name", span, path, [re.compile(re.escape(f"`{span}`"))])
    return removed


def searched_files(root: Path, everything: bool) -> list[str]:
    tracked = git(root, "ls-files", "--recurse-submodules", "--", *SEARCHED).splitlines()
    untracked = git(root, "ls-files", "--others", "--exclude-standard", "--", *SEARCHED).splitlines()
    files = sorted({path for path in tracked + untracked if (root / path).is_file()})
    if everything:
        return files
    return [path for path in files if not re.search(r"(^|/)docs/design/20|(^|/)review-lessons\.md$", path)]


def search(root: Path, files: list[str], patterns: list[re.Pattern[str]]) -> list[str]:
    hits = []
    for path in files:
        for number, line in enumerate(new_text(root, path).splitlines(), start=1):
            if any(pattern.search(line) for pattern in patterns):
                hits.append(f"{path}:{number}: {line.strip()[:160]}")
    return hits


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--base", help="ref whose merge base with HEAD is compared (default: origin/main, else main)")
    parser.add_argument("--term", action="append", default=[], help="an old wording to search for, case-insensitively")
    parser.add_argument("--all", action="store_true", help="also search dated design documents and review lessons")
    parser.add_argument("--strict", action="store_true", help="exit with 1 when there is a hit")
    args = parser.parse_args()

    try:
        root = Path(git(Path.cwd(), "rev-parse", "--show-toplevel").strip())
        base = merge_base(root, args.base)
        removed = collect(root, base)
        files = searched_files(root, args.all)
    except (RuntimeError, OSError) as error:
        print(f"check_copies: {error}", file=sys.stderr)
        return 2

    for term in args.term:
        removed[("term", term)] = Removed("term", term, {"--term"}, [re.compile(re.escape(term), re.I)])

    found = 0
    for entry in sorted(removed.values(), key=lambda item: (item.kind, item.name)):
        hits = search(root, files, entry.patterns)
        if not hits:
            continue
        found += len(hits)
        print(f"{entry.kind} {entry.name!r}, removed from {', '.join(sorted(entry.sources))}, still named in:")
        for hit in hits[:SHOWN]:
            print(f"  {hit}")
        if len(hits) > SHOWN:
            print(f"  ... and {len(hits) - SHOWN} more")
    print(f"{found} place(s) still name {len(removed)} removed name(s), compared with {base[:12]}"
          + ("" if args.all else "; dated design documents and review lessons skipped"))
    return 1 if args.strict and found else 0


if __name__ == "__main__":
    sys.exit(main())
