#include <kinetra/simulation/right_of_way.h>

#include <stdexcept>

namespace kinetra {
std::vector<RightOfWayEvent> RightOfWay::step(std::span<const StallMover> movers,
                                              const StallObservation &observation,
                                              const RightOfWaySettings &settings, std::uint64_t now) {
    if (settings.window_steps == 0)
        throw std::invalid_argument("Right of way requires a positive window");
    validate(observation, movers);
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
    for (std::size_t i = 0; i < movers.size(); ++i) {
        const auto &mover = movers[i];
        if (until_[i] || mover.state != StallMoverState::observed ||
            mover.no_progress < observation.threshold_steps)
            continue;
        if (const auto pusher = pushing_neighbor(movers, i, observation)) {
            until_[i] = now + settings.window_steps;
            events.push_back({RightOfWayEvent::Kind::granted, i, pusher});
        }
    }
    return events;
}
} // namespace kinetra
