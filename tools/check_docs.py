#!/usr/bin/env python3
"""Check relative Markdown links and heading anchors in a Git repository.

Usage: python3 tools/check_docs.py [repository-root]

Scans tracked and untracked-but-not-ignored ``*.md`` files (submodule contents are
not scanned, but links into a checked-out submodule are resolved). Reports links
whose target file is missing and ``#anchor`` fragments that match no heading,
using GitHub's heading slug rules. External URLs are not fetched.
Exit status: 0 when every link resolves, 1 otherwise.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote

LINK = re.compile(r"!?\[(?:[^\[\]]|\[[^\]]*\])*\]\((<[^>]+>|[^)\s]+)(?:\s+\"[^\"]*\")?\)")
HEADING = re.compile(r"^ {0,3}(#{1,6})\s+(.*?)\s*#*\s*$")
FENCE = re.compile(r"^ {0,3}(`{3,}|~{3,})")
INLINE_CODE = re.compile(r"(`+)(?:(?!\1).)+?\1")
EXTERNAL = ("http://", "https://", "mailto:", "ftp://")


def markdown_files(root: Path) -> list[Path]:
    """Tracked plus untracked, non-ignored Markdown files below root."""
    output = subprocess.run(
        ["git", "-C", str(root), "ls-files", "--cached", "--others", "--exclude-standard", "--", "*.md"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    return sorted({root / line for line in output.splitlines() if line and (root / line).is_file()})


def prose_lines(text: str):
    """Yield (line number, line) outside fenced code blocks, with inline code removed."""
    fence = None
    for number, line in enumerate(text.splitlines(), start=1):
        match = FENCE.match(line)
        if match:
            marker = match.group(1)
            if fence is None:
                fence = marker[0] * len(marker)
            elif marker.startswith(fence):
                fence = None
            continue
        if fence is None:
            yield number, INLINE_CODE.sub("", line)


def slug(heading: str) -> str:
    """GitHub-style anchor for a heading's rendered text."""
    text = re.sub(r"!?\[([^\]]*)\]\([^)]*\)", r"\1", heading)  # links -> their text
    text = text.replace("`", "")
    text = re.sub(r"<[^>]+>", "", text)  # inline HTML
    text = re.sub(r"[*_~]{1,3}(\S(?:.*?\S)?)[*_~]{1,3}", r"\1", text)  # emphasis markers
    text = text.strip().lower()
    text = re.sub(r"[^\w\- ]", "", text)
    return text.replace(" ", "-")


_anchor_cache: dict[Path, set[str]] = {}


def anchors(path: Path) -> set[str]:
    """All heading anchors of a Markdown file, including GitHub's -1, -2 suffixes."""
    if path not in _anchor_cache:
        seen: dict[str, int] = {}
        result: set[str] = set()
        for _, line in prose_lines(path.read_text(encoding="utf-8")):
            match = HEADING.match(line)
            if not match:
                continue
            base = slug(match.group(2))
            count = seen.get(base, 0)
            seen[base] = count + 1
            result.add(base if count == 0 else f"{base}-{count}")
        _anchor_cache[path] = result
    return _anchor_cache[path]


def check(root: Path) -> list[str]:
    problems: list[str] = []
    for source in markdown_files(root):
        text = source.read_text(encoding="utf-8")
        for number, line in prose_lines(text):
            for match in LINK.finditer(line):
                target = match.group(1).strip("<>")
                if target.startswith(EXTERNAL) or target.startswith("data:"):
                    continue
                path_part, _, fragment = target.partition("#")
                resolved = source if not path_part else (source.parent / unquote(path_part)).resolve()
                where = f"{source.relative_to(root)}:{number}"
                if not resolved.exists():
                    problems.append(f"{where}: missing target {target}")
                    continue
                if fragment and resolved.is_file() and resolved.suffix == ".md":
                    if unquote(fragment).lower() not in anchors(resolved):
                        problems.append(f"{where}: missing anchor #{fragment} in {path_part or source.name}")
    return problems


def main(argv: list[str]) -> int:
    root = Path(argv[1] if len(argv) > 1 else ".").resolve()
    top = subprocess.run(
        ["git", "-C", str(root), "rev-parse", "--show-toplevel"], check=True, capture_output=True, text=True
    ).stdout.strip()
    root = Path(top)
    problems = check(root)
    for problem in problems:
        print(problem)
    count = len(markdown_files(root))
    if problems:
        print(f"{len(problems)} broken link(s) in {count} Markdown files under {root}")
        return 1
    print(f"All links resolve in {count} Markdown files under {root}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
