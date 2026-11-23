# verify

Use `verify` to find out which checks a change needs and what actually passed. The report names the commands, results and any checks still blocked or pending.

## Table of contents

- [Use cases](#use-cases)
- [When not to use it](#when-not-to-use-it)
- [How it works](#how-it-works)
- [Related skills](#related-skills)

[Skills overview](../README.md) · [Instructions for agents](SKILL.md) · [Project overview](../../../README.md) · [Documentation index](../../../docs/README.md)

## Use cases

| Situation | Example request | Result |
| --- | --- | --- |
| Only a skill README changed | “/verify Controleer de links van de nieuwe skillgids.” | The Docs check reports broken file links and heading anchors or confirms that they resolve. |
| A CI job is renamed | “/verify Ik heb de `ubuntu`-job in de matrix opgenomen.” | The copies check lists every page that still names the old job (“job `ubuntu`”, “CI / ubuntu”), before review finds them. |
| One private simulation implementation changed mid-branch | “/verify Controleer de wijziging in de gewogen zoekroutine.” | A targeted build and the relevant CTest cases, provided the rest of the diff also qualifies. |
| A public navigation header changed | “/verify Controleer de branch met de gewijzigde navigatie-API.” | Targeted builds and tests of the changed areas locally, then the pull request's CI results, graphics cases included. |
| The settings panel's layout changed | “/verify Ik heb de rijen van het settings-paneel verschoven.” | Besides the checks, a screenshot for the pull request and the captures that the media guides name for the panel, refreshed from Kinetra-Lab: `ui-settings` and `ui-inspector`, and no other GIF. |

## When not to use it

For a judgement about whether an implementation meets its goals, use `pr-review` on the open pull request. For a bug that needs a fix, start with `feature`.

## How it works

1. The agent examines committed, uncommitted and untracked changes in each affected repository. The highest-impact file determines the level. Final verification of a code branch uses Full.
2. At every level it runs `tools/check_copies.py`, which lists names the diff removes that a page still names, and every old wording given with `--term`; each hit is judged. Then it runs the checks for that level. For Full it also checks the pull request's CI and reports any remaining checks as pending. See the [verification levels](../../../docs/testing.md#verification-levels). A change to what a scene shows also gets a screenshot and refreshes the captures that the media guides name for it ([rule](../../../AGENTS.md#repository-documentation)).
3. You get a result per command, with the failing test or blocker where applicable.

## Related skills

[Feature](../feature/README.md) uses verification before handover, [refactor](../refactor/README.md) after each step, [doc-module](../doc-module/README.md) for documentation and [pr-feedback](../pr-feedback/README.md) after review fixes.
