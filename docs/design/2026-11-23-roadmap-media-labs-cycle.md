# Roadmap: the shared UI in GIFs, images in the launcher menu and a leaner cycle

Status: implemented · Branch: `claude/roadmap-overlays-gifs-agents-jdz3yc` · Related: [roadmap](../roadmap.md), [Lab design](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-23-roadmap-media-labs-cycle.md)

On 2026-11-23 the user asked for six additions to the roadmaps. Three need Kinetra: GIFs of the shared UI in this README, images in the launcher menu for a preview of each lab, and a leaner agentic cycle, whose rules and skills live here. This branch adds them to Kinetra's roadmap; the Lab design lists all six wishes, the facts checked and the decisions.

## Table of contents

- [Problem](#problem)
- [Decisions](#decisions)
- [Design](#design)
- [Verification](#verification)
- [Pull request](#pull-request)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem

Kinetra's README shows no media, though the launcher and the overlays it describes are the shared UI of every lab. The launcher menu cannot show a lab's description or a picture of it: `LaunchChoice::description` is not drawn, and the runtime has no images. The agent rules, the workflow and the skills that set the cost of every change live in this repository.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Sections | *Leaner agentic cycle* first in *Next* and *Shared UI in the README* at its end; *Images in the launcher menu* at the end of *Later* | The user put the cycle first on 2026-11-23; the README item has a concrete first step in Lab's design; the images need a design with a possible new dependency |
| GIFs and Demo | *Shared UI in the README* depends on Lab's *Demo scenario menu* | The user wants the configuration page shown with Demo, which has none yet |
| Authoritative item | Lab's items hold the scenes, the measurements and the candidate measures; Kinetra's items hold Kinetra's part and link them | One place per concept |

## Design

[Roadmap](../roadmap.md): **Leaner agentic cycle** first in *Next*, **Shared UI in the README** at its end, **Images in the launcher menu** in *Later*, each linked to its Lab item.

## Verification

- `python3 tools/check_docs.py`: all links resolve.

## Pull request

**docs(roadmap): the shared UI in GIFs, images in the launcher menu and a leaner cycle**

- **Summary:** Kinetra's part of the user's wishes of 2026-11-23 as roadmap items.
- **Changes:** the roadmap, this design document and the index.
- **Design:** this document and the Lab design.
- **Verification:** Docs level.
- **Dependencies and merge order:** merges before the Lab pull request of the same branch, with a merge commit.
- **Follow-up:** the items themselves.
