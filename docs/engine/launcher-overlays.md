# Launcher and overlays

Header-only core UI helpers provide shared layout, capture and primitive drawing. The optional `Kinetra::Launcher` target supplies generic menus and session hosting; [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/launcher-overlays.md) registers concrete applications.

## Table of contents

- [Launcher](#launcher)
- [Contracts](#contracts)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Launcher

[`kinetra/launcher.h`](../../include/kinetra/launcher.h) exposes `LaunchMenu`, shared `MenuLayout` geometry, `SessionHost` and `run_launcher`. Enabling `KINETRA_BUILD_RUNTIME` also builds `Kinetra::Launcher`, which publicly links Runtime. Consumers choose whether to link it; direct `run_session` use remains supported.

The menu title is KINETRA LAUNCHER; application rows appear without a selection subtitle. Moving the pointer over a row preselects it without confirming; a resting pointer preserves arrow-key selection. Footer actions use the same blue hover highlight as Settings.

The centered menu displays up to six entries per page. Previous and Next buttons wrap pages; arrow-key selection reveals the corresponding page. Exit sets `LaunchMenu::exit_requested()`, which custom loops must consume.

Applications supply names, configuration labels and session factories. Hosts own one session, preserve the current session/camera if a factory fails, and suppress selection input and elapsed time on the first advance. The standard runner uses M to return to its menu; applications can instead implement their own loop with `SessionHost`.

`tests/launcher_tests.cpp` covers menu selection, validation, transition isolation and failed factories; the external consumer checks public header/target linkage. Build and test with the [development commands](../development.md).

## Contracts

[DebugMenu](../../include/kinetra/debug_view.h) accepts labels, borrowed values, actions and observational getters. Applications keep referenced state alive through update/draw and suppress gameplay pointer input when the menu captures it. Backquote is the default toggle. Actionable rows turn blue while the pointer is inside their click region; section labels and read-only values retain their normal color.

[HelpOverlay](../../include/kinetra/help_view.h) and `draw_help` accept application-owned content and style. Help does not capture gameplay input. [Bitmap text](../../include/kinetra/bitmap_text.h) uses embedded glyphs and runtime primitives.

`LaunchMenu` optionally accepts a heading at construction and a status message when drawing. Defaults preserve the standard launcher. Applications can reuse its pagination for local resource selection without exposing their resource types to Core.

`DebugMenu::screen_origin` optionally fixes the panel origin in screen pixels; a negative x retains right alignment.
