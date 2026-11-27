#pragma once

#include <kinetra/simulation/passing.h>
#include <kinetra/simulation/physics.h>
#include <kinetra/simulation/spatial_grid.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace kinetra {
/// What a mover is to the coordinator in one step: whether it may join or keep a plan, and
/// what kind of obstacle it is to the plans of others.
enum class PassingMoverState {
    /// Follows a route of its own: joins a plan when its heading is not zero, and keeps one.
    /// To other plans a moving obstacle that the priority rule may let them ignore.
    free,
    /// Moves under another control (its goal changed, another solver or the player drives
    /// it): joins no plan and ends its own as PassingOutcome::unavailable. A moving obstacle,
    /// as `free`.
    busy,
    /// Stands still for now, such as a parked, held or sleeping mover: joins no plan and ends
    /// its own as PassingOutcome::unavailable. Blocks a remaining leg it stands on whatever
    /// the priority.
    standing,
    /// Does not exist in this step, such as an empty slot of a pool: no obstacle, joins no
    /// plan and ends its own as PassingOutcome::unavailable. It loses any retry delay, so the
    /// next occupant of its index starts afresh. A caller with a pool keeps its indices
    /// stable this way.
    absent,
};

/// One mover as a PassingCoordinator sees it at the start of a step. The caller's time unit
/// is one step: velocities are in world units per step.
struct PassingMover {
    /// The mover's body; it must pass validate(), also for an absent mover. Of it the
    /// coordinator uses its bounds() and velocity: any supported shape takes part through its
    /// footprint, the axis-aligned box around its bounds centered on its position.
    Body body;
    /// Direction of the mover's route at its position, of length one or zero; zero when it
    /// follows none. Only a free mover with a heading joins a new plan.
    Vec2 heading;
    /// Final goal of the trip in world coordinates; a lane exit keeps room before it.
    Vec2 destination;
    /// Its part in this step.
    PassingMoverState state{PassingMoverState::free};
};

/// Tuning of a PassingCoordinator, in world units and steps. The caller chooses the scale;
/// there are no defaults.
struct PassingCoordination {
    /// Lane geometry, as passing_lane_candidates() takes it. `margins.gap` is also the gap
    /// of the conflict test.
    PassingMargins margins;
    /// Speed and acceleration limits per step, the horizon in steps over which conflicts are
    /// predicted, and the sample interval of the lane guard.
    PassingPrediction prediction;
    /// Added to each participant's half extents in the lane sweeps and the guard.
    Vec2 clearance;
    /// A participant within this distance of its target and slower than `stop_speed`
    /// counts as arrived there. Positive.
    float arrival_distance;
    /// Speed per step below which an arriving participant counts as standing. Positive.
    float stop_speed;
    /// Steps the lane ahead of a waiting participant must stay clear before it drives on.
    std::uint64_t resume_steps;
    /// Longest wait, and longest stretch without progress toward the targets, before a plan
    /// ends. Positive.
    std::uint64_t timeout_steps;
    /// Steps after a plan that did not complete before its participants may join another.
    std::uint64_t retry_steps;
};

/// Why a plan ended.
enum class PassingOutcome {
    /// Both participants reached their exits.
    completed,
    /// A participant was no longer free (busy, standing or absent) or left the movers of a step.
    unavailable,
    /// A stationary mover stands on a remaining leg, an oncoming one drives into the lane,
    /// or a wait lasted `timeout_steps`.
    obstructed,
    /// The summed distance to the targets did not shrink for `timeout_steps`.
    no_progress,
    /// The caller ended it with cancel() or clear().
    cancelled,
};

/// What happened to a plan in a step.
enum class PassingEventKind {
    /// Two movers agreed on a plan; its lanes are set.
    formed,
    /// Both participants reached their entries and start traversing.
    crossing,
    /// The plan ended; its outcome is set.
    ended,
};

/// One change of a plan, in the order in which the coordinator made them.
struct PassingEvent {
    PassingEventKind kind;
    /// The plan's id: 1 for the first plan of a coordinator, increasing.
    std::uint64_t plan;
    /// Mover indices of the participants, in the order of their lanes.
    std::array<std::size_t, 2> movers;
    /// The agreed lanes, one per participant.
    PassingLanes lanes;
    /// For PassingEventKind::ended, why the plan ended.
    std::optional<PassingOutcome> outcome;
};

/// The point a participant heads for during this step.
struct PassingTarget {
    /// Its braking point while it waits, otherwise its entry and, once crossing, its exit.
    Vec2 point;
    /// Whether it waits for a mover with priority to leave its lane.
    bool waiting;
};

/// The state of one active plan, for drawing and inspection.
struct PassingPlanState {
    /// The plan's id, as PassingEvent::plan names it.
    std::uint64_t id;
    /// Mover indices of the participants, in the order of `lanes`.
    std::array<std::size_t, 2> movers;
    /// The agreed lanes, one per participant.
    PassingLanes lanes;
    /// Both participants reached their entries.
    bool crossing;
    /// Per participant, whether it waits.
    std::array<bool, 2> waiting;
};

/// Checks that a participant may sweep its footprint, half extents `half_extents` with the
/// clearance included, along one leg from `from` to `to` through the caller's world:
/// terrain, walls, zones. Other movers and the space of other plans are checked by the
/// coordinator. `mover` is the participant's index. An empty function approves every leg.
using PassingLaneCheck = std::function<bool(std::size_t mover, Vec2 from, Vec2 to, Vec2 half_extents)>;

/// Coordinates head-on passing between movers that follow routes. Two movers whose route
/// headings will bring them into contact agree on a pair of side lanes; each drives to its
/// entry, and once both stand there, they cross to their exits. The coordinator owns the
/// plans and their timing; the caller steers each participant to target() and replans its
/// route after the plan ends.
///
/// In every step it guards each plan against the movers ahead of each participant on its
/// remaining legs (PassingLaneGuard). A participant waits at its braking point while a
/// mover with priority is predicted to enter its lane ahead, and resumes after the lane
/// stayed clear for `resume_steps`. A plan yields to movers in no plan and to the
/// participants of older plans. It ends when a participant is no longer free, when it is
/// obstructed, when it makes no progress, or when both participants reached their exits.
/// Plans never share swept space.
///
/// Mover indices are positions in the span of a step. A caller that renumbers its movers
/// calls clear() first; one with a pool of slots marks empty slots absent, and cancels a
/// slot's plan when a new occupant takes it between two steps. Results are deterministic for
/// the same inputs. Not thread-safe.
class PassingCoordinator {
  public:
    /// Advances the plans by one step, then forms new ones among `movers`. Returns the
    /// events of the step in order: ended and crossing plans first, then formed ones.
    /// `lanes_clear` decides each leg of a candidate against the world. Throws
    /// std::invalid_argument before changing anything for invalid settings, or for a mover
    /// with an invalid body, a nonfinite destination, a heading of a length other than zero or
    /// one, or a state outside PassingMoverState; nothing after that check throws.
    std::vector<PassingEvent> step(std::span<const PassingMover> movers, const PassingCoordination &settings,
                                   const PassingLaneCheck &lanes_clear);
    /// Ends the plan of `mover`, if any, as PassingOutcome::cancelled and returns its event.
    /// Its participants, and `mover` in any case, may join a new plan at once.
    std::vector<PassingEvent> cancel(std::size_t mover);
    /// Ends every plan as PassingOutcome::cancelled, in plan order, and returns the events.
    /// Every retry delay ends too, so a caller that renumbers its movers starts afresh.
    std::vector<PassingEvent> clear();

    /// The point `mover` heads for in its plan, if it takes part in one.
    std::optional<PassingTarget> target(std::size_t mover) const;
    /// The active plans, oldest first.
    std::vector<PassingPlanState> plans() const;
    /// Number of active plans.
    std::size_t size() const { return plans_.size(); }

  private:
    /// The footprints of the movers that are not absent in a spatial index.
    struct Neighbors {
        SpatialGrid grid;
        float cell;
        /// Bounds and mover index of each indexed footprint.
        std::vector<Aabb> boxes;
        std::vector<std::size_t> ids;
        /// False when a footprint lies too far from the origin for the grid's coordinates.
        bool use_grid;
        bool within_grid(Aabb area) const;
        /// Mover indices whose footprints overlap `area`, ascending. An area wider than the
        /// grid answers cheaply is scanned box by box, so no query throws.
        std::vector<std::size_t> query(Aabb area) const;
    };
    struct Plan {
        std::uint64_t id{};
        std::array<std::size_t, 2> movers{};
        PassingLanes lanes{};
        /// Swept areas of both participants' legs, reserved against other plans.
        std::vector<Aabb> space;
        bool crossing{};
        /// Least summed distance to the current targets, and the step it was reached.
        float best_remaining{};
        std::uint64_t progress_at{};
        std::array<bool, 2> waiting{};
        /// Per participant: step its wait began, and the last step a mover with priority
        /// was predicted in its lane ahead.
        std::array<std::uint64_t, 2> waiting_since{}, intruded_at{};
        /// Per participant: where it stops while it waits.
        std::array<Vec2, 2> hold{};
    };
    std::vector<Plan> plans_;
    /// Per mover: first step at which it may join a new plan.
    std::vector<std::uint64_t> retry_;
    std::uint64_t next_plan_{1};
    std::uint64_t step_{};

    void advance(std::span<const Body> footprints, std::span<const PassingMover> movers,
                 const PassingCoordination &settings, const Neighbors &index, Vec2 largest_half,
                 std::vector<PassingEvent> &events);
    std::optional<PassingOutcome> guard(Plan &plan, std::span<const Body> footprints,
                                        std::span<const PassingMover> movers,
                                        const PassingCoordination &settings, const Neighbors &index,
                                        float margin) const;
    bool stalled(Plan &plan, std::span<const Body> footprints, const PassingCoordination &settings);
    bool yields_to(const Plan &plan, std::size_t other) const;
    void form(std::span<const Body> footprints, std::span<const PassingMover> movers,
              const PassingCoordination &settings, const PassingLaneCheck &lanes_clear,
              const Neighbors &index, Vec2 largest_half, std::vector<PassingEvent> &events);
    bool reserve(Plan &plan, std::span<const Body> footprints, const PassingCoordination &settings,
                 const PassingLaneCheck &lanes_clear, const Neighbors &index) const;
    /// Removes plan `index` with `outcome`; a failed plan keeps its participants out of new
    /// plans for `retry_steps`.
    void end(std::size_t index, PassingOutcome outcome, std::uint64_t retry_steps,
             std::vector<PassingEvent> &events);
};
} // namespace kinetra
