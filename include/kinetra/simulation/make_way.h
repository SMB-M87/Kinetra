#pragma once
#include <kinetra/simulation/math.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace kinetra {
/// Limits for a make-way request. Step counts and the search limit must be positive.
struct MakeWaySettings {
    /// Steps before a trip times out and starts checking for a return.
    std::uint64_t timeout_steps;
    /// Minimum steps between unsuccessful searches in the same episode.
    std::uint64_t retry_steps;
    /// Maximum number of searches in the same episode.
    unsigned max_searches;
    /// A standing blocker closer to home than this distance has returned.
    float home_tolerance;
};

/// Application motion state, with an operator hold taking priority.
enum class MakeWayMotion { idle, moving, held, unreachable };

/// State of one mover in the application's world units.
struct MakeWayMover {
    Vec2 position;
    Vec2 destination;
    /// The yielding mover's reserved return position.
    Vec2 home;
    MakeWayMotion motion;
    /// Whether its current goal still belongs to MakeWay.
    bool yielding;
};

/// A neighbor in the way of a stalled mover. Even if it cannot yield, an alternate
/// route for the stalled mover may still be possible.
struct MakeWayBlocker {
    std::size_t mover;
    /// Whether the application permits assigning a temporary route to this neighbor.
    bool can_yield;
};

/// A hypothetical substitute position for a blocker when checking a beneficiary's route.
struct MakeWayRelocation {
    std::size_t mover;
    Vec2 position;
};

/// The application may apply different clearance rules to each route purpose.
enum class MakeWayRoute { beneficiary, yielding };
/// Why an active request ended.
enum class MakeWayEnd {
    /// The beneficiary has cleared the space it needed.
    cleared,
    /// The policy or one of the participants withdrew from the request.
    cancelled,
    /// The beneficiary still needed space when the deadline passed.
    timed_out,
    /// The checked yielding route became unavailable.
    obstructed,
};

/// An outcome is emitted once per trip. It can precede the actual return home.
struct MakeWayEvent {
    enum class Kind { started, unavailable, ended };
    Kind kind;
    std::size_t beneficiary;
    std::size_t blocker;
    /// Present only for an ended request.
    std::optional<MakeWayEnd> outcome;
};

/// World policy and commands used by MakeWay during one step. Queries read live state
/// so an earlier command in the same step can affect a later request. Do not reenter
/// MakeWay from a callback. Paths and positions use the same world units throughout.
class MakeWayWorld {
  public:
    virtual ~MakeWayWorld() = default;

    /// The live state of a mover. Index is less than the count passed to step().
    virtual MakeWayMover mover(std::size_t index) const = 0;
    /// Start step of the current stall episode, or no episode.
    virtual std::optional<std::uint64_t> stall(std::size_t beneficiary) const = 0;
    /// The neighbor blocking the beneficiary, or no blocker.
    virtual std::optional<MakeWayBlocker> blocker(std::size_t beneficiary) const = 0;
    /// Possible refuge positions, in application order for equally distant candidates.
    virtual std::vector<Vec2> refuges(std::size_t blocker) const = 0;
    /// A checked route ending at target, or an empty route if unavailable.
    virtual std::vector<Vec2> route(std::size_t mover, Vec2 target, MakeWayRoute purpose,
                                    std::optional<MakeWayRelocation> relocated = {}) const = 0;
    /// Whether returning to the reserved home would block the beneficiary's route.
    virtual bool needs_home(std::size_t beneficiary, std::size_t blocker) const = 0;
    /// Whether a live policy or reservation invalidates the yielding mover's route.
    virtual bool route_forbidden(std::size_t blocker) const = 0;
    /// Whether a stationary obstacle now obstructs the yielding mover's route.
    virtual bool route_obstructed(std::size_t blocker) const = 0;
    /// Installs a nonempty route. Update mover() to reflect the command before returning.
    virtual void assign(std::size_t mover, std::vector<Vec2> route, MakeWayRoute purpose) = 0;
    /// Starts braking a yielding mover in place without releasing its temporary goal.
    virtual void brake(std::size_t blocker) = 0;
    /// Releases a yielding mover that is standing at home.
    virtual void returned(std::size_t blocker) = 0;
    /// Reports a transition synchronously, in the order it happened.
    virtual void event(MakeWayEvent event) = 0;
};

/// Coordinates temporary yielding for existing stall episodes. A request tries an
/// alternate route before moving a neighbor to the nearest checked refuge. Trips
/// end when cleared, cancelled, obstructed or timed out, then return by a checked
/// route as the world permits. Existing trips advance before new requests, each in
/// mover order. Disabling the policy stops new requests and lets existing trips return.
///
/// Indices identify the same movers across steps. Call clear() before reusing IDs
/// or resetting the caller's clock. Concurrent calls require external synchronization.
class MakeWay {
  public:
    /// Advances the lifecycle at the caller's monotonically increasing step index.
    /// Indices left out by a smaller count cancel their requests. Callback views and
    /// results are used only during this call. Throws std::invalid_argument before
    /// any state change for invalid settings.
    void step(MakeWayWorld &world, std::size_t count, std::uint64_t now, bool enabled,
              const MakeWaySettings &settings);
    /// Forget all trips and searches. The caller resets any goal it assigned from them.
    void clear();

  private:
    enum class Phase { outbound, waiting, returning };

    struct Trip {
        std::size_t beneficiary;
        Vec2 destination;
        std::uint64_t deadline;
        Phase phase{Phase::outbound};
        std::optional<MakeWayEnd> outcome{};
        bool route_invalidated{};
    };

    struct Attempt {
        std::uint64_t episode{};
        std::uint64_t retry_at{};
        unsigned searches{};
    };

    void advance_trip(MakeWayWorld &world, std::size_t index, std::size_t count, std::uint64_t now,
                      bool enabled, float home_tolerance);
    void start_request(MakeWayWorld &world, std::size_t beneficiary, std::uint64_t now,
                       const MakeWaySettings &settings);
    static void finish(MakeWayWorld &world, Trip &trip, std::size_t blocker, MakeWayEnd outcome);
    static bool return_home(MakeWayWorld &world, Trip &trip, std::size_t blocker, Vec2 home);

    std::vector<std::optional<Trip>> trips_;
    std::vector<Attempt> attempts_;
};
} // namespace kinetra
