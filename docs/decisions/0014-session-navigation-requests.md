# 0014: Session navigation requests

Status: accepted. Amends [0010](0010-generic-launcher-in-core.md): the standard launcher loop honors session requests and configurable bindings.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

A `Session` can only say `exit_requested()`. Kinetra-Lab's sessions also need to return to the launcher menu. Lab therefore keeps `lab::Session::return_to_menu()` and its own event loop instead of `run_launcher`, for three reasons:
- `run_launcher` never reads `exit_requested()`;
- it returns to the menu on a hard-coded M;
- it closes on Escape regardless of `Session::close_on_escape()`.

Directly launched sessions have a further problem. `run_session` ignores a return request, so the LAUNCHER button of standalone Demo and Dotio does nothing. Evidence and the full design: [settings overlay, glyphs and session navigation](../design/2026-11-18-overlay-glyphs-navigation.md).

## Decision

- **One request.** `enum class SessionRequest { none, menu, exit }` and `virtual SessionRequest request() const` replace `Session::exit_requested()`. Hosts read it after every advance and before render.
- **Standalone.** `run_session` ends the run on `menu` as on `exit`, because a directly launched session has no menu.
- **Launcher.** `run_launcher` returns to its menu on `menu` and ends on `exit`. A trailing `LauncherBindings` parameter makes the M binding optional and controls Escape in the menu; during a run, Escape follows `close_on_escape()`. The defaults keep the current behavior for existing callers.

Rejected alternatives:
- **A `menu_requested()` beside `exit_requested()`.** It would be additive and compatible, but two related flags encode one state, and a session could raise both.
- **Removing the M binding.** It would be simpler, but it changes behavior for other launchers.
- **Leaving navigation to each consumer's own loop.** That keeps Lab's duplicate loop and makes `SessionHost` reuse partial.

## Consequences

- **Breaking change.** Consumers that override `exit_requested()` must switch to `request()`. Overrides marked `override` stop compiling, so the migration cannot be missed. Kinetra-Lab is the only known consumer and adapts on its phase 3 branch, together with the pointer bump.
- **Lab.** Lab can drop its launcher loop, `lab::Session` and i4sim's standalone wrapper. Its standalone LAUNCHER button ends the program, as i4sim's does today.
- **Behavior fix.** `run_launcher` honors `close_on_escape()`. A session that opts out of Escape no longer loses its window to it.
- **Tests.** The graphics runtime tests cover both hosts' handling of each request and both bindings. The external consumer links the new `run_launcher` signature.
- **Docs.** When this is implemented, [runtime](../engine/runtime.md) and [launcher and overlays](../engine/launcher-overlays.md) describe the requests, and this ADR's status drops "not yet implemented".
