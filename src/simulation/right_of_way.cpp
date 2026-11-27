#include <kinetra/simulation/right_of_way.h>

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
    for (std::size_t i = movers.size(); i < until_.size(); ++i)
        if (until_[i])
            events.push_back({RightOfWayEvent::Kind::ended, i, std::nullopt});
    until_.resize(movers.size());
    for (std::size_t i = 0; i < movers.size(); ++i)
        if (until_[i] && (now >= *until_[i] || movers[i].state != StallMoverState::observed)) {
            until_[i].reset();
            events.push_back({RightOfWayEvent::Kind::ended, i, std::nullopt});
        }
    // A window that would run past the clock's range lasts to its end instead of wrapping.
    const auto end = now > std::numeric_limits<std::uint64_t>::max() - settings.window_steps
                         ? std::numeric_limits<std::uint64_t>::max()
                         : now + settings.window_steps;
    for (std::size_t i = 0; i < movers.size(); ++i) {
        const auto &mover = movers[i];
        if (until_[i] || mover.state != StallMoverState::observed ||
            mover.no_progress < observation.threshold_steps || !pushers[i])
            continue;
        until_[i] = end;
        events.push_back({RightOfWayEvent::Kind::granted, i, pushers[i]});
    }
    return events;
}
} // namespace kinetra
