# 0013: Settings overlay in Kinetra

Status: accepted. Kinetra owns the modal settings panel; applications supply its rows.

## Table of contents

- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Context

Kinetra-Lab's Demo, Dotio and i4sim share a centered settings panel with these parts:
- a compact SETTINGS/DEBUG toolbar;
- zoom, Statistics and Debug Mode controls;
- collapsible help;
- Launcher, Resume and Exit actions.

The panel lives in Lab twice: `lab::Settings` and a near-identical `i4sim::FactorySettings`. A `bool dotio` argument selects help lines, and a height field must match them by hand. The panel contains no game rules. It is input capture, a layout shared by picking and drawing, and drawing, which is the kind of mechanism [DebugMenu](../../include/kinetra/runtime/debug_view.h) and [HelpOverlay](../../include/kinetra/runtime/help_view.h) already provide in Kinetra. Evidence and the full design: [settings overlay, glyphs and session navigation](../design/2026-11-18-overlay-glyphs-navigation.md).

## Decision

Kinetra gains `SettingsOverlay` in `Kinetra::Runtime`, built from application-supplied rows: a zoom pair with limits read at click time, toggles over borrowed flags, and help sections with their own lines. Kinetra owns:
- the title, the toolbar, the action row and the hint;
- a layout derived from the rows, which reproduces today's panel for Lab's rows;
- reporting the actions as a [`SessionRequest`](0014-session-navigation-requests.md).

Rejected alternatives:
- **A fixed layout with only application help text.** It removes the copy, but no other consumer could add a row.
- **Reusing `DebugOption` rows.** One row type for both overlays would be neat, but it changes the visible layout and invalidates every settings test.
- **Keeping the panel in Lab.** It would only remove the i4sim copy, and the mechanism would stay outside the engine.

## Consequences

- **Easier.** A new application gets the standard settings panel by listing rows. The help height can no longer drift from the help lines.
- **Harder.** The row kinds are now public API. A new kind of control needs a Kinetra change, and its own decision when it alters the layout rules.
- **Lab.** `lab::Settings` and `FactorySettings` are removed on Lab's phase 3 branch, and the pointer bump adapts the three sessions.
- **Tests.** Lab's settings tests move to Kinetra and assert today's coordinates for a Demo-like and a Dotio-like row set. The external consumer links the new header.
- **Contract page.** When this is implemented, [launcher and overlays](../engine/launcher-overlays.md) describes the overlay, and this ADR's status drops "not yet implemented".
