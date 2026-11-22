#pragma once

#include <kinetra/view/input.h>
#include <kinetra/view/inspector.h>

namespace kinetra {
class Runtime;

/// What the read-only inspector did with this frame's pointer.
struct InspectorInput {
    /// The pointer is over the panel. The caller suppresses world pointer actions.
    bool captured{};
    /// The close button was clicked. The caller clears the selection.
    bool close{};
};

/// Scrollable inspector anchored at the top right of a caller-provided area. All geometry
/// uses logical window pixels. The application owns the selection and the snapshot.
class InspectorView {
  public:
    /// The rectangle used for drawing and pointer capture, or a zero-sized box if the
    /// available area cannot fit a title bar and a row.
    Aabb bounds(const InspectorSnapshot &snapshot, Aabb area) const;
    /// Consume the pointer inside the panel and scroll on wheel input.
    InspectorInput update(const Input &input, const InspectorSnapshot &snapshot, Aabb area);
    /// Draw the snapshot inside an active runtime frame, using the same area as update.
    void draw(Runtime &runtime, const InspectorSnapshot &snapshot, Aabb area) const;
    /// Return to the first row when the application changes or clears its selection.
    void reset();

  private:
    double first_line_{};
    Vec2 pointer_{-1, -1};
};
} // namespace kinetra
