#include <kinetra/simulation/right_of_way.h>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace kinetra {
std::vector<RightOfWayEvent> RightOfWay::step(std::span<const StallMover> movers,
                                              const StallObservation &observation,
                                              const RightOfWaySettings &settings, std::uint64_t now) {
    if (settings.window_steps == 0)
        throw std::invalid_argument("Right of way requires a positive window");
    // Validates the movers and the tuning once, before anything changes.
    const auto pushers = pushing_neighbors(movers, observation);
    std::vector<RightOfWayEvent> events;
    // A right of way ends with its window, with the trip it was granted for (a new goal), or
    // when its mover is no longer observed; the indices a shorter span leaves out come last,
    // so the ends stay in mover order.
    for (std::size_t i = 0; i < std::min(movers.size(), grants_.size()); ++i) {
        const auto &grant = grants_[i];
        if (grant && (now >= grant->until || movers[i].destination != grant->destination ||
                      movers[i].state != StallMoverState::observed)) {
            grants_[i].reset();
            events.push_back({RightOfWayEvent::Kind::ended, i, std::nullopt});
        }
    }
    for (std::size_t i = movers.size(); i < grants_.size(); ++i)
        if (grants_[i])
            events.push_back({RightOfWayEvent::Kind::ended, i, std::nullopt});
    grants_.resize(movers.size());
    // A window that would run past the clock's range lasts to its end instead of wrapping.
    const auto end = now > std::numeric_limits<std::uint64_t>::max() - settings.window_steps
                         ? std::numeric_limits<std::uint64_t>::max()
                         : now + settings.window_steps;
    for (std::size_t i = 0; i < movers.size(); ++i) {
        const auto &mover = movers[i];
        if (grants_[i] || mover.state != StallMoverState::observed ||
            mover.no_progress < observation.threshold_steps || !pushers[i])
            continue;
        grants_[i] = Grant{end, mover.destination};
        events.push_back({RightOfWayEvent::Kind::granted, i, pushers[i]});
    }
    return events;
}
} // namespace kinetra
