#!/usr/bin/env python3
"""Check first-party extensions and local Markdown links without extra packages."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = {'.git', 'vendor', 'third_party', 'external', 'node_modules', 'CMakeFiles', 'glad', 'KHR'}
FORBIDDEN = {'.hpp', '.hxx', '.hh', '.cc', '.cxx'}

def maintained(path):
    return not any(part in EXCLUDED or (part.lower() == 'build' or part.startswith('build-')) for part in path.relative_to(ROOT).parts)

def anchors(text):
    result = set()
    counts = {}
    # Ignore headings inside fenced examples and Mermaid.
    text = re.sub(r'```.*?```', '', text, flags=re.S)
    for heading in re.findall(r'^#{1,6}\s+(.+?)\s*#*$', text, flags=re.M):
        anchor = re.sub(r'[^\w\- ]', '', heading.lower()).replace(' ', '-')
        count = counts.get(anchor, 0)
        counts[anchor] = count + 1
        result.add(anchor + (f'-{count}' if count else ''))
    return result

def check():
    errors = []
    files = [p for p in ROOT.rglob('*') if p.is_file() and maintained(p)]
    for path in files:
        if path.suffix in FORBIDDEN:
            errors.append(f'{path.relative_to(ROOT)}: forbidden first-party C++ extension')
        if path.suffix != '.md':
            continue
        text = path.read_text()
        if '## Table of contents' not in text:
            errors.append(f'{path.relative_to(ROOT)}: missing table of contents')
        for target in re.findall(r'(?<!!)\[[^\]]*\]\(([^)]+)\)', text):
            if re.match(r'[a-zA-Z]+:', target):
                continue
            target = target.split(' "', 1)[0].strip('<>')
            file, _, fragment = target.partition('#')
            destination = (path.parent / file).resolve() if file else path
            if not destination.exists():
                errors.append(f'{path.relative_to(ROOT)}: missing {target}')
            elif fragment and destination.suffix == '.md' and fragment not in anchors(destination.read_text()):
                errors.append(f'{path.relative_to(ROOT)}: missing anchor {target}')
    return errors

if __name__ == '__main__':
    errors = check()
    print('\n'.join(errors) if errors else 'PASS first-party extensions, Markdown TOCs and local links')
    sys.exit(bool(errors))
