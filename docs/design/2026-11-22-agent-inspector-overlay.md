# Agent inspector overlay

Status: implemented · [Runtime roadmap](../roadmap.md#next)

## Problem

Lab applications can draw steering vectors and paths. To understand one moving agent, the operator also needs its current goal, state and motion settings. The existing debug menu supplies controls for an entire session. An inspector needs a way to present read-only details for one selection while the simulation runs.

## Design

The application supplies a title and ordered groups of label–value rows. Each value includes its display units. The snapshot owns its strings so a headless caller can keep it after the next tick. Selection, agent IDs, sampling time and the meaning of each row belong to the application.

The runtime draws one snapshot in the top-right corner. A close button in the fixed title bar consumes its click and signals the application to clear the selection. Replacing the snapshot with another agent's details reuses the same panel. Keep the debug menu on the left usable at the same time.

Use the same logical-pixel bounds for drawing and pointer capture. A long snapshot needs scrolling within the panel, including after a resize. Wheel input over the panel scrolls the rows. Wheel input over the world keeps the application's zoom binding. Opening settings gives settings input priority. When the frame-time graph is visible, the application reserves space below the inspector for the graph and its captions.

## Affected areas and test plan

- Add the plain snapshot alongside the [debug menu model](../../include/kinetra/view/debug_menu.h). Exercise wrapped rows, scroll limits and close-button input without a window.
- Add a runtime view alongside the [debug view](../../include/kinetra/runtime/debug_view.h). Exercise viewport edges, a long selection followed by a short one, and pointer capture inside and outside the drawn bounds. Test the close action after scrolling and resizing, with the debug menu and the frame-time graph visible.
- Exercise drawing with the offscreen graphics preset. Build the public headers as an external consumer.

The first consumer is i4sim. Its CLI reads the same snapshot directly. The engine has no dependency on a factory blueprint, a simulation tick or a serialization format.

## Review

Drawing and hit testing share the same bounds. Round the available rectangle inward before layout so fractional positions cannot leak clicks from painted edges. The title and close button stay in place while wrapped rows scroll. Tests now observe a visible continuation and reset the scroll position for a shorter selection. The consumer calls the public layout and input API without a window.

### Review of `06bdf68`

In `1b7aeec`, the resize test starts at the scroll limit, enlarges the view and checks the visible continuation and final label before clicking close. The combined layout test draws the debug menu, inspector, graph bars and caption, then picks both controls. A separate presentation test verifies string ownership, order and formatted units with the runtime disabled.

## Verification

- `cmake --workflow --preset graphics` passed all 280 tests with SDL offscreen and software Mesa.
- The external graphics consumer built and ran with `CONSUMER_GRAPHICS=ON`.
- `python3 tools/check_docs.py` resolved every link.
- `cmake --build --preset graphics --target kinetra_engine_runtime_tests kinetra_presentation_tests` and `ctest --preset graphics -R 'inspector|frame.time graph' --output-on-failure` passed all five targeted cases after review.
- `cmake -S . -B /tmp/kinetra-inspector-headless -G Ninja -DKINETRA_BUILD_RUNTIME=OFF` and `cmake --build /tmp/kinetra-inspector-headless --target kinetra_presentation_tests` built the snapshot test without the runtime. `ctest --test-dir /tmp/kinetra-inspector-headless -R 'headless inspector snapshots' --output-on-failure` passed it.

## Pull request

**Title** `feat(runtime): add a shared agent inspector overlay`

Add a headless model for grouped diagnostic rows and a scrollable runtime view with a close action. Applications own selection and pause behavior. Lab adds the three consumers, beginning with [i4sim in Lab #74](https://github.com/SMB-M87/Kinetra-Lab/pull/74).
