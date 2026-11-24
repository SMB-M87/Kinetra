# Shared UI in GIFs

The GIFs behind the README's [shared UI](../../README.md#shared-ui) section: the launcher menu and the overlays every Kinetra-Lab application gets from `Kinetra::Launcher` and the runtime, what each GIF shows, and when and how to refresh them. Kinetra has no applications of its own, so the GIFs come from Kinetra-Lab's Demo, rendered by Lab's [capture tool](https://github.com/SMB-M87/Kinetra-Lab/blob/main/tools/capture/README.md).

## Table of contents

- [Files](#files)
- [Capture details](#capture-details)
- [Which GIFs a change makes stale](#which-gifs-a-change-makes-stale)
- [Refreshing the GIFs](#refreshing-the-gifs)

[Documentation index](../README.md) · [Project overview](../../README.md#shared-ui)

## Files

| File | Shows |
| --- | --- |
| [ui-launcher.gif](ui-launcher.gif) | `LaunchMenu`: hovering changes the selected lab's image and description, DEMO opens its configuration page, and DOORWAY starts that scenario through the `SessionHost` |
| [ui-settings.gif](ui-settings.gif) | `SettingsOverlay` in Demo's Doorway: Ctrl opens it and Demo pauses, STATISTICS goes off and on, SIM CONTROLS and DEBUG CONTROLS open their help lines and close them, and RESUME closes the panel |
| [ui-debug-menu.gif](ui-debug-menu.gif) | `DebugMenu` in Doorway: Shift+Ctrl opens it, the `[+]` stepper of SPEED and of MOTION's MAX SPEED, AVOIDANCE choosing LEGACY and then RECIPROCAL again, the LAYERS section with its swatches, and Shift+Ctrl closing it |
| [ui-inspector.gif](ui-inspector.gif) | `InspectorView` in Doorway: a click on an agent opens it at the top right, FRAME TIMES in the settings panel puts the frame-time graph (`draw_frame_times`) below it, a click on another agent switches it, and its X closes it |
| [ui-debug-layers.gif](ui-debug-layers.gif) | Demo's debug layers in Terrain, with agents that wander by planned routes: VECTORS, STEERING PROBES, PATHS, COLLISION SHAPES and ACTIVE AGENTS off and on again, HOME RANGE on, and the wheel zooming out over the streamed terrain |

## Capture details

- **Rendering.** Lab's `kinetra_capture` drives the real launcher and Demo session with a scripted user, as Lab's tour video does, and draws the pointer, click rings and keycaps on each frame after it was read back. The applications draw nothing of this.
- **Size and rate.** 960 × 540, the size of Lab's clips, so the bitmap font stays sharp and the overlays are large; 15 frames per second, every fourth frame of a 60 Hz run. Each GIF is about 1 MB or less.
- **Frame times.** The capture calls `Runtime::fix_frame_times(1.0 / 60)`, so every frame counts as one 60 Hz refresh and the graph reads 16.7 ms in every capture; the offscreen driver reports VSync, so it names VSYNC as the pacing ([runtime](../engine/runtime.md#sessions-and-timing)). A real run shows its own frame times.
- **Reproducibility.** Seed 42, fixed steps and software rendering: two refreshes on the same machine give the same frames. Another machine can differ in a few pixels, through another Mesa or ffmpeg, so only GIFs that a change makes stale are refreshed (below).

## Which GIFs a change makes stale

A pull request that changes what a GIF shows refreshes that GIF in the same pull request ([rule](../../AGENTS.md#repository-documentation)). Refresh the GIFs of every row that the change touches; commit no other GIF, because a refresh on another machine changes pixels without a reason.

| A change to | Makes stale |
| --- | --- |
| `LaunchMenu` and `MenuLayout` ([launcher.h](../../include/kinetra/runtime/launcher.h), `src/runtime/launcher.cpp`) | `ui-launcher` |
| `SettingsOverlay` and the toolbar ([settings_view.h](../../include/kinetra/runtime/settings_view.h), `src/runtime/settings_view.cpp`) | `ui-settings`, `ui-inspector` |
| `DebugMenu` and its rows ([debug_view.h](../../include/kinetra/runtime/debug_view.h), [debug_menu.h](../../include/kinetra/view/debug_menu.h)) | `ui-debug-menu`, `ui-debug-layers` |
| `InspectorView` ([inspector_view.h](../../include/kinetra/runtime/inspector_view.h), `src/runtime/inspector_view.cpp`) or the frame-time graph (`draw_frame_times`) | `ui-inspector` |
| Bitmap text, the overlay colors ([bitmap_text.h](../../include/kinetra/runtime/bitmap_text.h), [overlay_style.h](../../include/kinetra/runtime/overlay_style.h)) or the runtime's drawing ([engine.h](../../include/kinetra/runtime/engine.h)) | Every GIF here, and Lab's captures ([Lab's table](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/media/README.md#which-captures-a-change-makes-stale)) |

A change in Kinetra-Lab that alters only Demo in these GIFs, such as its scene, its rows or its colors, does not need a Kinetra pull request: these GIFs are about the shared UI, and they follow such a change with the next Kinetra change that makes them stale. A row that a script clicks by its label and that was renamed stops the capture, so Lab's scripts follow the rename in the Lab pull request.

## Refreshing the GIFs

The capture runs in Kinetra-Lab, with its `external/Kinetra` submodule on the Kinetra branch of the change, so the GIFs show the engine as the branch changes it:

```sh
# In Kinetra-Lab, with external/Kinetra checked out at the Kinetra branch
cmake --preset graphics
cmake --build --preset graphics --target kinetra_capture
tools/capture/capture_media.sh build-graphics ui-settings ui-inspector   # the stale GIFs
```

The script needs ffmpeg and writes the GIFs into `external/Kinetra/docs/media`, this directory in the submodule. Look at the frames of every refreshed GIF before committing it: a script that clicks the wrong place shows a scene that does not match its row above. Commit the GIFs on the Kinetra branch, in the Kinetra pull request; they need no Lab pull request of their own, and Lab's pointer follows the Kinetra merge as usual. Keep the file names in sync with this guide and with the README; Lab's `kinetra_capture_tests` checks both.
