# Debug and help overlays

Header-only core UI helpers provide shared layout, capture and primitive drawing. Registered menus and session hosts live in [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/launcher-overlays.md).

## Table of contents

- [Contracts](#contracts)

## Contracts

[DebugMenu](../../include/kinetra/debug_view.h) accepts labels, borrowed values, actions and observational getters. Applications keep referenced state alive through update/draw and suppress gameplay pointer input when the menu captures it. Backquote is the default toggle.

[HelpOverlay](../../include/kinetra/help_view.h) and `draw_help` accept application-owned content and style. Help does not capture gameplay input. [Bitmap text](../../include/kinetra/bitmap_text.h) uses embedded glyphs and runtime primitives.
