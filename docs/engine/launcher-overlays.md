# Debug and help overlays

Header-only core UI helpers provide shared layout, capture and primitive drawing. [launcher.h](../../include/kinetra/launcher.h) provides `LaunchMenu`, `SessionHost` and `run_launcher` through `Kinetra::Launcher`. Applications supply value-owned choices and session factories; Kinetra owns selection, session lifetime and the standard event loop. Failed factories preserve the previous session/camera. Successful transitions suppress input and elapsed time on the first update. The bundled registration and usage examples live in [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/launcher-overlays.md).

## Table of contents

- [Contracts](#contracts)
- [Navigation](#navigation)

## Contracts

[DebugMenu](../../include/kinetra/debug_view.h) accepts labels, borrowed values, actions and observational getters. Applications keep referenced state alive through update/draw and suppress gameplay pointer input when the menu captures it. Backquote is the default toggle.

[HelpOverlay](../../include/kinetra/help_view.h) and `draw_help` accept application-owned content and style. Help does not capture gameplay input. [Bitmap text](../../include/kinetra/bitmap_text.h) uses embedded glyphs and runtime primitives.

## Navigation

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)
