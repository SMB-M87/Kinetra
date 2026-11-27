#pragma once
#include <kinetra/simulation/physics.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace kinetra {
/// What a mover is to the stall detector in one step.
enum class StallMoverState {
    /// Follows a route of its own, which the caller wants watched: the detector observes it,
    /// and to others it is a moving neighbor that can be an opposing or a pushing mover.
    observed,
    /// Moves under another control (a wait, a passing plan, a temporary trip): not observed,
    /// and its episode ends. To others a moving neighbor, never an opposing one.
    busy,
    /// Stands still for now, such as a parked or held mover: not observed, and to others a
    /// stationary blocker when it stands on their leg.
    standing,
    /// Does not exist in this step, such as an empty slot of a pool: not observed and no
    /// neighbor. Its observation starts afresh when the index is next observed. A movers span
    /// shorter than the last step's ends the episodes of the indices it leaves out.
    absent,
};

/// One mover as the detector sees it after a step. Lengths are in world units and
/// velocities in world units per step, as the caller chooses them.
struct StallMover {
    /// The mover's body; it must pass validate(), also for an absent mover, as the target
    /// and destination must be finite. Its footprint is the axis-aligned box around its
    /// bounds, centered on its position.
    Body body;
    /// The point the mover heads for now, such as the next waypoint of its route. Its leg
    /// runs from the body's position to this point.
    Vec2 target;
    /// The final goal of the trip; a changed goal starts a new observation.
    Vec2 destination;
    /// Consecutive steps without progress along the trip, as the caller measures progress:
    /// zero after a step with progress. A count that does not grow starts a new observation.
    std::uint64_t no_progress{};
    /// Its part in this step.
    StallMoverState state{StallMoverState::observed};
};

/// Tuning of a StallDetector, in world units and steps. There are no defaults.
struct StallObservation {
    /// Observed steps without progress after which an episode starts. Positive.
    std::uint64_t threshold_steps;
    /// A displacement that counts as a move along an axis, a reversal or a sideways
    /// excursion. Positive.
    float excursion;
    /// Reversals of at least `excursion` that make an episode oscillating. Positive.
    unsigned oscillation_reversals;
    /// Distance beyond twice the mover's half diagonal over which its leg is checked for
    /// blockers: what it can cover before it reacts and stops. Nonnegative.
    float lookahead;
    /// Added to the mover's half extents when its leg is swept. Nonnegative.
    Vec2 clearance;
    /// Speed along a leg at or below which a mover makes no headway, for opposing movers,
    /// and the backward speed above which a mover is pushed back. Positive.
    float stop_speed;
    /// Cosine of the angle between two legs below which the movers oppose each other, in
    /// (-1, 0).
    float opposing_cosine;
    /// A pushing neighbor lies within this many times the length of the two half extents
    /// combined. Positive.
    float pushing_reach;
    /// Cosine of the angle off the mover's leg within which a pushing neighbor lies ahead,
    /// in [0, 1).
    float ahead_cosine;
};

/// Why the detector thinks a mover stalls, from the evidence at the onset of the episode.
enum class StallReason {
    /// A standing mover lies on the mover's leg within reach; `blocker` names it.
    stationary_blocker,
    /// An observed mover blocks the leg, and the mover blocks its leg in turn, on opposed
    /// legs, and neither makes headway; `blocker` names it.
    opposing_mover,
    /// The mover moves back along its leg while a moving neighbor ahead, within reach,
    /// drives toward it: it is pushed away from its target; `blocker` names the neighbor.
    pushed_back,
    /// The mover reversed at least `oscillation_reversals` times.
    oscillating,
    /// None of the above applied.
    unknown,
};

/// The diagnosis of one episode, made when it starts and kept until it ends.
struct StallDiagnosis {
    /// The `now` of the step in which the episode started.
    std::uint64_t detected_at{};
    StallReason reason{StallReason::unknown};
    /// Mover index of a stationary blocker, an opposing mover or a pushing neighbor.
    std::optional<std::size_t> blocker{};
};

/// A change of one mover's episode.
struct StallEvent {
    enum class Kind {
        /// The episode started; its diagnosis is set.
        started,
        /// The episode ended: progress resumed, the goal changed, or the mover is no longer
        /// observed. The diagnosis is the one made at the start.
        ended,
    };
    Kind kind;
    std::size_t mover;
    StallDiagnosis diagnosis;
};

/// Throws std::invalid_argument for settings outside their documented ranges, or for a mover
/// with an invalid body, a nonfinite target or destination, or a state outside
/// StallMoverState: what StallDetector::observe() and RightOfWay::step() reject.
void validate(const StallObservation &settings, std::span<const StallMover> movers);

/// The first neighbor, in mover order, that pushes `movers[index]` back: the mover moves back
/// along its leg faster than `stop_speed`, and the neighbor, neither standing nor absent, lies
/// ahead within `ahead_cosine` of the leg, within `pushing_reach` times the length of the two
/// half extents combined, and drives toward the mover faster than `stop_speed`. Nothing when
/// the mover does not back away or no neighbor pushes. The detector's `pushed_back` reason
/// uses this test. Throws as validate() does, and std::out_of_range for an `index` outside
/// `movers`.
std::optional<std::size_t> pushing_neighbor(std::span<const StallMover> movers, std::size_t index,
                                            const StallObservation &settings);

/// pushing_neighbor() for every mover at once, validating once: entry `i` names the neighbor
/// pushing `movers[i]` back, if any. Throws as validate() does.
std::vector<std::optional<std::size_t>> pushing_neighbors(std::span<const StallMover> movers,
                                                          const StallObservation &settings);

/// Watches movers that follow routes and reports sustained lack of progress as episodes,
/// each with the evidence at its onset: a stationary blocker or an opposing mover on its
/// leg, a neighbor pushing it back, oscillation, or nothing found.
///
/// The caller measures progress; the detector observes the motion while the progress count
/// grows. An observation of a mover starts afresh when it is not observed, has no progress
/// count, its goal changes or its count does not grow. Within an observation the detector
/// sets an axis once the mover has moved `excursion`, counts a reversal when it backs up
/// `excursion` from its farthest point along the axis, and takes a new axis after a sideways
/// excursion. After `threshold_steps` observed steps an episode starts with a diagnosis; it
/// ends when the observation restarts.
///
/// Mover indices are positions in the span of a step; a caller with a pool marks empty
/// slots absent. Results are deterministic for the same inputs. Not thread-safe.
class StallDetector {
  public:
    /// Observes the movers after a step at the caller's clock value `now`, and returns the
    /// episodes that ended or started: first those of movers beyond a shorter span than the
    /// last step's, which end, then the others in mover order. Throws
    /// std::invalid_argument before changing anything for invalid settings, or for a mover
    /// with an invalid body, a nonfinite target or destination, or a state outside
    /// StallMoverState.
    std::vector<StallEvent> observe(std::span<const StallMover> movers, const StallObservation &settings,
                                    std::uint64_t now);
    /// The diagnosis of `mover`'s active episode, if it has one.
    std::optional<StallDiagnosis> episode(std::size_t mover) const;
    /// Ends every observation without events, so a caller that renumbers its movers starts
    /// afresh.
    void clear() { tracks_.clear(); }

  private:
    /// The observation of one mover.
    struct Track {
        /// Whether an observed step anchored the track; until then the goal and anchor
        /// mean nothing.
        bool started{};
        Vec2 goal{};
        /// Reference point and axis of the current excursion; the axis is zero until the
        /// mover has moved `excursion` from the anchor.
        Vec2 anchor{}, axis{};
        /// Farthest displacement from the anchor along the axis.
        float forward_peak{};
        unsigned reversals{};
        std::uint64_t observations{}, last_no_progress{};
        std::optional<StallDiagnosis> episode;
        void observe_position(Vec2 position, float excursion);
    };
    std::vector<Track> tracks_;
};
} // namespace kinetra
