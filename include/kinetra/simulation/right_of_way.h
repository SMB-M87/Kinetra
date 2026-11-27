#pragma once
#include <kinetra/simulation/stall_detector.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace kinetra {
/// Tuning of a RightOfWay, in steps. There are no defaults.
struct RightOfWaySettings {
    /// Steps a mover keeps the right of way once it has it. Positive.
    std::uint64_t window_steps;
};

/// A change of one mover's right of way.
struct RightOfWayEvent {
    enum class Kind {
        /// The mover got the right of way; `pusher` names the neighbor pushing it back.
        granted,
        /// Its window passed, or it was no longer observed.
        ended,
    };
    Kind kind;
    std::size_t mover;
    /// For `granted`, the neighbor that pushed the mover back.
    std::optional<std::size_t> pusher;
};

/// A temporary motion fallback for a mover pushed away from its goal: a stalled mover that a
/// neighbor drives back gets the right of way for a fixed window. While it has it, the caller
/// keeps steering it at its target and tells its other movers not to count on it to make room,
/// so the pushing neighbor avoids it alone.
///
/// Each step the caller passes the movers as for StallDetector. An observed mover whose steps
/// without progress reach `StallObservation::threshold_steps` and that pushing_neighbor()
/// finds pushed back gets the right of way until `window_steps` steps later. It loses it
/// earlier when it is no longer observed: it arrived, stopped, changed to another control or is
/// absent. After the window it can get it again at once if it is still stalled and pushed back.
/// The fallback measures no progress itself and changes no motion; the caller applies it.
///
/// Mover indices are positions in the span of a step; a span shorter than the last step's ends
/// the right of way of the indices it leaves out. Results are deterministic for the same inputs.
/// Not thread-safe.
class RightOfWay {
  public:
    /// Ends the rights of way whose window passed or whose mover is no longer observed, then
    /// grants new ones, at the caller's clock value `now`; returns the events in that order,
    /// each part in mover order. Throws std::invalid_argument before changing anything for a
    /// zero window or for what validate() rejects.
    std::vector<RightOfWayEvent> step(std::span<const StallMover> movers, const StallObservation &observation,
                                      const RightOfWaySettings &settings, std::uint64_t now);
    /// Whether `mover` has the right of way after the last step.
    bool granted(std::size_t mover) const { return mover < until_.size() && until_[mover].has_value(); }
    /// Ends every right of way without events, so a caller that renumbers its movers starts
    /// afresh.
    void clear() { until_.clear(); }

  private:
    /// Per mover, the step at which its right of way ends.
    std::vector<std::optional<std::uint64_t>> until_;
};
} // namespace kinetra
