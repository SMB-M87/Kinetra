#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace kinetra {
/// Result of one TrafficGate::update().
struct TrafficAdmission {
    /// The agent allowed into the zone: the single physical occupant, the persisting grant,
    /// or the oldest eligible request. Empty when nobody may enter.
    std::optional<std::uint64_t> agent;
    /// Requesting agents other than the admitted one.
    std::size_t waiting{};
    /// True when more than one agent physically occupies the zone; nobody is admitted then.
    bool occupied_conflict{};
};

/// Exclusive, occupancy-aware FIFO admission for one caller-defined conflict zone.
/// Update once per control step with complete request/physical-occupancy snapshots.
/// Newly requesting IDs join in sorted order; persistent eligible requests retain priority.
/// Physical occupants block new admission even after cancelling their request.
/// Multiple occupants receive no admission, but callers must allow them to leave.
/// No timeout revokes an approaching grant: callers must cancel only when they can
/// safely stop outside. Fair progress requires each admitted agent to clear the zone.
/// Geometry, braking, zone selection and multi-zone deadlock prevention stay external.
class TrafficGate {
  public:
    /// Updates the queue from the snapshots and returns the admission; every request is
    /// eligible. Throws std::invalid_argument, before changing state, when a snapshot
    /// contains duplicate IDs.
    TrafficAdmission update(std::span<const std::uint64_t> requests,
                            std::span<const std::uint64_t> occupants);
    /// Temporarily ineligible requests retain their FIFO tickets. New admission
    /// selects the oldest eligible request; an existing grant is never revoked.
    /// ineligible must contain unique IDs from requests. Invalid input leaves state intact.
    TrafficAdmission update(std::span<const std::uint64_t> requests, std::span<const std::uint64_t> occupants,
                            std::span<const std::uint64_t> ineligible);
    /// Forgets the queue and the grant, for example when the policy is disabled or
    /// replaced. Does not move anything.
    void clear();

  private:
    std::vector<std::uint64_t> queue_;
    std::optional<std::uint64_t> admitted_;
};
} // namespace kinetra
