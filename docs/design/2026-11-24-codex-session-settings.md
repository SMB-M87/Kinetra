# Codex session settings

Status: implemented · Branch: `chore/codex-session-settings`

Kinetra's [Claude settings](../../.claude/settings.json) preapprove the GitHub calls used in review and start the Ubuntu toolchain setup in the cloud. Codex needs its own project configuration for the GitHub connector and an entry point to run the same setup.

## Table of contents

- [Goals and scope](#goals-and-scope)
- [Decisions](#decisions)
- [Design](#design)
- [Affected areas](#affected-areas)
- [Test plan](#test-plan)
- [Review](#review)
- [Verification](#verification)
- [Pull request](#pull-request)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Goals and scope

- Let Codex check repository access and CI, read pull requests and reviews, and post reviews, comments and replies and resolve threads in a trusted Kinetra checkout.
- Let Ubuntu cloud runners reuse Kinetra's dependency setup and report its result through the setup phase. Keep Claude start and resume behavior, as well as the Lab wrapper's extra package.

Repository grants come from the connected GitHub account. The operator selects the setup script and connector approvals for each managed environment.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Review permissions | Use Codex's project-scoped approval settings for the GitHub connector | The trusted checkout gets approvals for the review workflow. The connected account supplies repository access. |
| Ubuntu cloud setup | Accept an explicit `--cloud` option in the shared script and return exit 1 if setup fails | A cloud runner can reuse the package list and formatter pin and report a failed setup. Claude's hook still returns 0 to deliver its diagnostic line. |

## Design

`.codex/config.toml` names the GitHub connector and the tools that correspond to Claude's GitHub review calls. `AGENTS.md` and the skills define how to use them. Each managed environment controls repository access and starts the toolchain setup through its own mechanism.

`tools/cloud_setup.sh` starts when Claude reports a remote session or when another runner passes `--cloud`. Extra packages continue to pass through to the existing package check. The script reports failures through an exit code for the explicit setup call and through output for the Claude hook. The tool guide explains the cloud environment setup and the workflow explains which approvals apply.

## Affected areas

- Project approval settings in `.codex/config.toml` and their use in the [review workflow](../workflow.md#review-access).
- Ubuntu environment setup in `tools/cloud_setup.sh`, its regression check in `tools/tests/check_cloud_setup.sh` and the [tool guide](../../tools/README.md#cloud_setupsh).

## Test plan

- Read the effective config and skill list with Codex's app server. Compare trusted and untrusted runs. Include repository access and CI tools among the configured approvals.
- Stub package queries for the local call, the existing Claude invocation and the explicit cloud option with an extra package. An apt failure must reach the runner as exit 1 and reach the Claude agent as a diagnostic line with exit 0.
- Run the documentation and copies checks, the focused checks above and the branch CI for Full verification.

## Review

The [first review](https://github.com/SMB-M87/Kinetra/pull/59#pullrequestreview-6038574875) found missing reads for repository authorization and CI, and a setup exit code that hid failures from cloud runners. The config now covers those reads. The explicit setup surfaces failures as exit 1, with a regression test for both exit paths. The design records the scope, verification commands and environment operator instructions.

Codex uses a single review call for a summary and its inline comments. The workflow's `COMMENT` rule governs that call.

The [second review](https://github.com/SMB-M87/Kinetra/pull/59#pullrequestreview-6038681942) confirmed both goals and resolved the four threads after CI passed on `e489af5`.

## Verification

- Codex CLI `0.167.0-alpha.4` loaded the 17 approvals only in the trusted checkout. It discovered all nine Kinetra skills there. To inspect the config from Kinetra's root, start the app server with an isolated Codex configuration directory.

  ```sh
  kinetra_codex_check=$(mktemp -d)
  printf '[projects."%s"]\ntrust_level = "trusted"\n' "$PWD" > "$kinetra_codex_check/config.toml"
  env CODEX_HOME="$kinetra_codex_check" codex app-server --strict-config
  ```

  Send these JSON-RPC lines in order. Wait for a response only to messages with an `id`. Replace `/path/to/Kinetra` with the checkout's absolute path. Repeat with `untrusted` in the temporary config to check the project's approval layer is absent.

  ```jsonl
  {"id":1,"method":"initialize","params":{"clientInfo":{"name":"kinetra-check","version":"1"}}}
  {"method":"initialized"}
  {"id":2,"method":"config/read","params":{"cwd":"/path/to/Kinetra","includeLayers":true}}
  {"id":3,"method":"skills/list","params":{"cwds":["/path/to/Kinetra"]}}
  ```

- `bash -n tools/cloud_setup.sh` and `bash tools/tests/check_cloud_setup.sh` passed. The regression check failed with the initial `--cloud` implementation because it reported exit 0 after a simulated apt failure.
- `python3 tools/check_docs.py`, `python3 tools/check_copies.py --term 'CLAUDE_CODE_REMOTE'`, and `git diff --check` passed. The copies check reports the current description of `CLAUDE_CODE_REMOTE` in the tool guide.

## Pull request

**Title** `chore(agents): add Codex session settings`

**Summary** Add project-scoped GitHub review approvals for Codex and expose the existing Ubuntu cloud setup to other runners.

**Changes** Codex reads `.codex/config.toml` to preapprove both the permission and CI checks and the review tools in a trusted checkout. Cloud environment setup can call `bash tools/cloud_setup.sh --cloud` and detects installation failures from its exit code.

**Design** `docs/design/2026-11-24-codex-session-settings.md`

**Verification** Codex config and skill discovery, `bash -n tools/cloud_setup.sh`, `bash tools/tests/check_cloud_setup.sh`, documentation links and the copies check. Cloud package installation and the connector approval prompts were outside the local checks.

**Review notes** GitHub account permissions determine which repositories can receive a review. The project config applies only to the Codex GitHub app route. The review process uses the `COMMENT` action.

**Dependencies and merge order** This change applies from the Kinetra checkout.

**Environment setup** The tool guide documents how operators enable the shared script in a Codex cloud environment.
