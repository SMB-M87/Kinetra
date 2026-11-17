#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace kinetra {
struct TrafficAdmission {
    std::optional<std::uint64_t> agent;
    std::size_t waiting{};
    bool occupied_conflict{};
};

/// Exclusive, occupancy-aware FIFO admission for one caller-defined conflict zone.
/// Update once per control step with complete request/physical-occupancy snapshots.
/// Newly requesting IDs join in sorted order; persistent requests retain their turn.
/// Physical occupants block new admission even after cancelling their request.
/// Multiple occupants receive no admission, but callers must allow them to leave.
/// No timeout revokes an approaching grant: callers must cancel only when they can
/// safely stop outside. Fair progress requires each admitted agent to clear the zone.
/// Geometry, braking, zone selection and multi-zone deadlock prevention stay external.
class TrafficGate {
  public:
    TrafficAdmission update(std::span<const std::uint64_t> requests,
                            std::span<const std::uint64_t> occupants);
    void clear();

  private:
    std::vector<std::uint64_t> queue_;
    std::optional<std::uint64_t> admitted_;
};
} // namespace kinetra
