#include <kinetra/simulation/make_way.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace kinetra {
namespace {
std::uint64_t after(std::uint64_t now, std::uint64_t steps) {
    return now + std::min(steps, std::numeric_limits<std::uint64_t>::max() - now);
}

std::vector<Vec2> nearest_refuge(const MakeWayWorld &world, std::size_t blocker, std::size_t beneficiary,
                                 Vec2 destination) {
    const auto position = world.mover(blocker).position;
    float nearest = std::numeric_limits<float>::infinity();
    std::vector<Vec2> best;
    for (const auto target : world.refuges(blocker)) {
        const auto separation = distance(position, target);
        if (separation >= nearest)
            continue;
        auto refuge = world.route(blocker, target, MakeWayRoute::yielding);
        if (!refuge.empty() && !world
                                    .route(beneficiary, destination, MakeWayRoute::beneficiary,
                                           MakeWayRelocation{blocker, target})
                                    .empty()) {
            best = std::move(refuge);
            nearest = separation;
        }
    }
    return best;
}
} // namespace

void MakeWay::finish(MakeWayWorld &world, Trip &trip, std::size_t blocker, MakeWayEnd outcome) {
    if (!trip.outcome) {
        trip.outcome = outcome;
        world.event({MakeWayEvent::Kind::ended, trip.beneficiary, blocker, outcome});
    }
}

bool MakeWay::return_home(MakeWayWorld &world, Trip &trip, std::size_t blocker, Vec2 home) {
    auto route = world.route(blocker, home, MakeWayRoute::yielding);
    if (route.empty())
        return false;
    world.assign(blocker, std::move(route), MakeWayRoute::yielding);
    trip.phase = Phase::returning;
    return true;
}

void MakeWay::advance_trip(MakeWayWorld &world, std::size_t index, std::size_t count, std::uint64_t now,
                           bool enabled, float home_tolerance) {
    auto &entry = trips_[index];
    auto &trip = *entry;
    const auto mover = world.mover(index);
    if (!mover.yielding) {
        finish(world, trip, index, MakeWayEnd::cancelled);
        entry.reset();
        return;
    }
    if (mover.motion == MakeWayMotion::held) {
        finish(world, trip, index, MakeWayEnd::cancelled);
        return;
    }
    if (trip.phase == Phase::returning && mover.motion == MakeWayMotion::idle) {
        if (distance(mover.position, mover.home) < home_tolerance) {
            world.returned(index);
            entry.reset();
        } else {
            trip.phase = Phase::waiting;
        }
        return;
    }

    const auto beneficiary =
        trip.beneficiary < count ? std::optional{world.mover(trip.beneficiary)} : std::nullopt;
    const bool cancelled = !enabled || !beneficiary || beneficiary->motion == MakeWayMotion::held ||
                           beneficiary->destination != trip.destination;
    const bool protects = beneficiary && beneficiary->motion == MakeWayMotion::moving &&
                          world.needs_home(trip.beneficiary, index);
    if (cancelled)
        finish(world, trip, index, MakeWayEnd::cancelled);
    else if (beneficiary->motion == MakeWayMotion::idle || (trip.phase != Phase::outbound && !protects))
        finish(world, trip, index, MakeWayEnd::cleared);
    else if (now >= trip.deadline)
        finish(world, trip, index, MakeWayEnd::timed_out);

    const bool moving = mover.motion == MakeWayMotion::moving;
    const bool forbidden = moving && world.route_forbidden(index);
    if (forbidden) {
        finish(world, trip, index, MakeWayEnd::obstructed);
        trip.route_invalidated = true;
    }
    // An expired or invalidated request cannot keep home reserved for the
    // beneficiary forever. The route planner still checks its current footprint.
    const bool can_return = !protects || cancelled || trip.route_invalidated || now >= trip.deadline ||
                            trip.outcome == MakeWayEnd::cancelled;
    if ((trip.phase == Phase::waiting || (moving && trip.phase == Phase::outbound)) && trip.outcome &&
        can_return && return_home(world, trip, index, mover.home))
        return;

    if (moving) {
        if (trip.phase != Phase::waiting && (forbidden || world.route_obstructed(index))) {
            finish(world, trip, index, MakeWayEnd::obstructed);
            world.brake(index);
            trip.phase = Phase::waiting;
        }
        return;
    }
    if (trip.phase == Phase::outbound) {
        if (!trip.outcome) {
            auto route = world.route(trip.beneficiary, trip.destination, MakeWayRoute::beneficiary);
            if (!route.empty())
                world.assign(trip.beneficiary, std::move(route), MakeWayRoute::beneficiary);
        }
        trip.phase = Phase::waiting;
    }
}

void MakeWay::start_request(MakeWayWorld &world, std::size_t beneficiary, std::uint64_t now,
                            const MakeWaySettings &settings) {
    const auto episode = world.stall(beneficiary);
    if (!episode)
        return;
    auto &attempt = attempts_[beneficiary];
    if (attempt.episode != *episode)
        attempt = {.episode = *episode};
    if (attempt.searches >= settings.max_searches || now < attempt.retry_at)
        return;

    const auto neighbor = world.blocker(beneficiary);
    if (!neighbor || neighbor->mover >= trips_.size() || neighbor->mover == beneficiary)
        return;
    const auto index = neighbor->mover;
    if (trips_[index] || std::any_of(trips_.begin(), trips_.end(), [beneficiary](const auto &trip) {
            return trip && trip->beneficiary == beneficiary;
        }))
        return;

    ++attempt.searches;
    attempt.retry_at = after(now, settings.retry_steps);
    const auto destination = world.mover(beneficiary).destination;
    if (auto route = world.route(beneficiary, destination, MakeWayRoute::beneficiary); !route.empty()) {
        world.assign(beneficiary, std::move(route), MakeWayRoute::beneficiary);
        attempt.searches = settings.max_searches;
        return;
    }
    if (neighbor->can_yield) {
        if (auto route = nearest_refuge(world, index, beneficiary, destination); !route.empty()) {
            world.assign(index, std::move(route), MakeWayRoute::yielding);
            trips_[index] = Trip{beneficiary, destination, after(now, settings.timeout_steps)};
            attempt.searches = settings.max_searches;
            world.event({MakeWayEvent::Kind::started, beneficiary, index, std::nullopt});
            return;
        }
    }
    world.event({MakeWayEvent::Kind::unavailable, beneficiary, index, std::nullopt});
}

void MakeWay::step(MakeWayWorld &world, std::size_t count, std::uint64_t now, bool enabled,
                   const MakeWaySettings &settings) {
    if (settings.timeout_steps == 0 || settings.retry_steps == 0 || settings.max_searches == 0 ||
        !std::isfinite(settings.home_tolerance) || settings.home_tolerance <= 0)
        throw std::invalid_argument("Make way requires positive limits and a finite home tolerance");
    if (trips_.empty() && !enabled)
        return;

    for (std::size_t i = 0; i < std::min(count, trips_.size()); ++i)
        if (trips_[i])
            advance_trip(world, i, count, now, enabled, settings.home_tolerance);
    for (std::size_t i = count; i < trips_.size(); ++i)
        if (trips_[i])
            finish(world, *trips_[i], i, MakeWayEnd::cancelled);
    trips_.resize(count);
    attempts_.resize(count);
    if (enabled)
        for (std::size_t i = 0; i < count; ++i)
            start_request(world, i, now, settings);
}

void MakeWay::clear() {
    trips_.clear();
    attempts_.clear();
}
} // namespace kinetra
