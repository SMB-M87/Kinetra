# README icon

Status: implemented · Branch: `docs/readme-icon` · Related: [Lab design](https://github.com/SMB-M87/Kinetra-Lab/blob/docs/readme-icon/docs/design/2026-11-19-readme-icon.md), [window icon](2026-11-19-window-icon.md)

The Kinetra icon from the [window icon](2026-11-19-window-icon.md) change appears on every window, but not on the project's front page. The user asked for the icon at the top of the README in Kinetra and in Lab. This document covers the Kinetra half.

## Table of contents

- [Problem](#problem)
- [Goals and non-goals](#goals-and-non-goals)
- [Decisions](#decisions)
- [Design](#design)
- [Affected areas](#affected-areas)
- [Test plan](#test-plan)
- [Review](#review)
- [Verification](#verification)
- [Pull request](#pull-request)
- [Open points](#open-points)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem

- [README.md](../../README.md) starts with plain text; the icon in [resources/kinetra-icon.png](../../resources/kinetra-icon.png) is not shown anywhere in the documentation.

## Goals and non-goals

- Goal: the README opens with the Kinetra icon, rendered on GitHub and in local Markdown viewers.
- Non-goal: a new or larger logo. The existing 256 × 256 icon is reused as is.
- Non-goal: icons on other pages.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Placement | A centered block above the `# Kinetra` title | The common project-page layout; the title stays a Markdown heading, so its anchor and the page layout from the [documentation guide](../documentation.md#page-layout) are unchanged. |
| Source | Relative path to `resources/kinetra-icon.png` | One source for the icon, so the README cannot drift from the window icon; it renders on GitHub and offline. |
| Size | 128 × 128 pixels, with explicit `width` and `height` | Half the source size stays sharp on high-DPI screens; explicit dimensions avoid a layout jump while the image loads. |

## Design

A `<p align="center">` block with one `<img>` sits above the title. GitHub keeps `align`, `width`, `height` and `alt` on these tags. The `alt` text names the icon for screen readers and for viewers without images.

## Affected areas

- [README.md](../../README.md)
- This document and the [design index](README.md#index)

## Test plan

Docs level: `python3 tools/check_docs.py`. The image path is checked by hand, because the checker only follows Markdown links.

## Review

Reviewed the diff by hand: only the README header and the design documents changed, the `alt` text is present, and the image path exists. No findings.

## Verification

- `python3 tools/check_docs.py`: passed.
- `test -f resources/kinetra-icon.png`: the image exists at the referenced path.

## Pull request

**Title:** `docs(readme): show the Kinetra icon at the top`

**Summary:** the README opens with the Kinetra icon, centered above the title.

**Changes:** a centered `<img>` of `resources/kinetra-icon.png` (128 × 128) above `# Kinetra`; this design document.

**Design:** [docs/design/2026-11-19-readme-icon.md](https://github.com/SMB-M87/Kinetra/blob/docs/readme-icon/docs/design/2026-11-19-readme-icon.md)

**Verification:** `python3 tools/check_docs.py` passed; the image path exists. Docs level, no build.

**Review notes:** documentation only.

**Dependencies and merge order:** none. The Lab pull request uses the icon already on Kinetra `main` and can merge in either order.

**Follow-up:** none.

## Open points

- None.
