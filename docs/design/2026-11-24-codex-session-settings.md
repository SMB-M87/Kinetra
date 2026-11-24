# Codex session settings

Status: implemented · Branch: `chore/codex-session-settings`

Kinetra's [Claude settings](../../.claude/settings.json) preapprove the GitHub calls used in review and start the Ubuntu toolchain setup in the cloud. Codex needs its own project configuration for the GitHub connector and an entry point to run the same setup.

## Table of contents

- [Decisions](#decisions)
- [Design](#design)
- [Test plan](#test-plan)
- [Review](#review)
- [Verification](#verification)
- [Pull request](#pull-request)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Review permissions | Use Codex's project-scoped approval settings for the GitHub connector | The trusted checkout gets approvals for the review workflow. The connected account supplies repository access. |
| Ubuntu cloud setup | Accept an explicit `--cloud` option in the shared script | A cloud runner can reuse the package list and formatter pin without needing Claude's environment variables. |

## Design

`.codex/config.toml` names the GitHub connector and the tools that correspond to Claude's GitHub review calls. `AGENTS.md` and the skills define how to use them. Each managed environment controls repository access and starts the toolchain setup through its own mechanism.

`tools/cloud_setup.sh` starts when Claude reports a remote session or when another runner passes `--cloud`. Extra packages continue to pass through to the existing package check. The tool guide documents how to invoke the script from a Codex cloud environment and the workflow explains which approvals apply.

## Test plan

- Read the effective config and skill list with Codex's app server. Use isolated trusted and untrusted configurations to check approval scope.
- Check the shell script with `bash -n` and fake package queries. Exercise a local call, the existing Claude invocation and the explicit cloud option with an extra package.
- Run the docs and copies checks. The change affects agent configuration and its setup script.

## Review

Compared the approved tools with the existing Claude settings and with the Codex GitHub connector's tool names. Codex uses a single review call for a summary and its inline comments. The workflow's `COMMENT` rule still governs that call.

Checked that the script supports the existing Claude and Lab calls and that only the initial `--cloud` option is consumed. The explicit option can be used in any Ubuntu runner's setup phase.

## Verification

- Codex CLI `0.167.0-alpha.4` with `app-server --strict-config` loaded the approved tools in the trusted checkout and omitted them in the untrusted one. `skills/list` discovered all nine Kinetra skills.
- `bash -n tools/cloud_setup.sh` and runs with stubbed package queries verified the local no-op, the Claude path and the `--cloud` path with an extra package.
- `python3 tools/check_docs.py`, `python3 tools/check_copies.py --term 'CLAUDE_CODE_REMOTE'`, and `git diff --check` passed. The copies check reports the current description of `CLAUDE_CODE_REMOTE` in the tool guide.

## Pull request

**Title** `chore(agents): add Codex session settings`

**Summary** Add project-scoped GitHub review approvals for Codex and expose the existing Ubuntu cloud setup to other runners.

**Changes** Codex reads `.codex/config.toml` in a trusted checkout. Cloud environment setup can now call `bash tools/cloud_setup.sh --cloud`.

**Design** `docs/design/2026-11-24-codex-session-settings.md`

**Verification** Codex config and skill discovery, Bash syntax and setup paths with stubbed package queries, documentation links, and copies check. A live review and a cloud package installation were outside the local checks.

**Review notes** GitHub account permissions determine which repositories can receive a review. The project config applies only to the Codex GitHub app route. The review process uses the `COMMENT` action.

**Dependencies and merge order** This change applies from the Kinetra checkout.

**Follow-up** Enable the shared setup command in the Codex cloud environment that should run it.
