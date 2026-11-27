#include <kinetra/simulation/stall_detector.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace kinetra {
namespace {
bool nonnegative(float value) {
    return std::isfinite(value) && value >= 0;
}

bool positive(float value) {
    return std::isfinite(value) && value > 0;
}

void validate(const StallObservation &settings) {
    if (settings.threshold_steps == 0 || !positive(settings.excursion) ||
        settings.oscillation_reversals == 0 || !nonnegative(settings.lookahead) ||
        !nonnegative(settings.clearance.x) || !nonnegative(settings.clearance.y) ||
        !positive(settings.stop_speed) || !std::isfinite(settings.opposing_cosine) ||
        settings.opposing_cosine <= -1 || settings.opposing_cosine >= 0 ||
        !positive(settings.pushing_reach) || !std::isfinite(settings.ahead_cosine) ||
        settings.ahead_cosine < 0 || settings.ahead_cosine >= 1)
        throw std::invalid_argument(
            "Stall observation requires a positive threshold, excursion, reversal count, "
            "stop speed and pushing reach, a nonnegative lookahead and clearance, an "
            "opposing cosine in (-1, 0) and an ahead cosine in [0, 1)");
}

/// An axis-aligned box centered on the body's position that covers its bounds.
Body footprint(const Body &body) {
    const auto area = bounds(body);
    Body box;
    box.shape = Shape::box;
    box.motion = Motion::controlled;
    box.position = body.position;
    box.velocity = body.velocity;
    box.half_extents = {std::max(body.position.x - area.min.x, area.max.x - body.position.x),
                        std::max(body.position.y - area.min.y, area.max.y - body.position.y)};
    return box;
}

/// A mover's footprint with its leg and heading, for the diagnosis.
struct Leg {
    Body body;
    Vec2 target, heading;
};

/// Whether `other` lies ahead of `mover` on the part of its leg it can cover before it
/// reacts and stops. A neighbor on a later leg may have left by the time the mover gets
/// there, so only the leg within reach counts.
bool blocks_leg(const Leg &mover, const Body &other, const StallObservation &settings) {
    const auto start = mover.body.position;
    const auto delta = mover.target - start;
    if (dot(other.position - start, delta) <= 0)
        return false;
    const float reach = length(mover.body.half_extents) * 2 + settings.lookahead;
    return swept_box_overlaps(start, start + limited(delta, reach),
                              mover.body.half_extents + settings.clearance, other);
}

StallDiagnosis diagnose(std::span<const Leg> legs, std::span<const StallMover> movers, std::size_t index,
                        unsigned reversals, const StallObservation &settings, std::uint64_t now) {
    const auto &mover = legs[index];
    std::optional<std::size_t> opposing;
    for (std::size_t i = 0; i < legs.size(); ++i) {
        const auto state = movers[i].state;
        if (i == index || state == StallMoverState::absent || !blocks_leg(mover, legs[i].body, settings))
            continue;
        if (state == StallMoverState::standing)
            return {now, StallReason::stationary_blocker, i};
        if (opposing || state != StallMoverState::observed)
            continue;
        const auto &other = legs[i];
        // Reciprocal avoidance can keep a pair moving sideways in place. Judge speed along
        // each leg to tell that from making headway.
        if (std::abs(dot(mover.body.velocity, mover.heading)) <= settings.stop_speed &&
            std::abs(dot(other.body.velocity, other.heading)) <= settings.stop_speed &&
            dot(mover.heading, other.heading) < settings.opposing_cosine &&
            blocks_leg(other, mover.body, settings))
            opposing = i;
    }
    if (opposing)
        return {now, StallReason::opposing_mover, opposing};
    // A mover near its goal can be pushed away from it by a neighbor whose route runs
    // through its place: both creep along, so neither stands nor opposes the other.
    if (dot(mover.body.velocity, mover.heading) < -settings.stop_speed)
        for (std::size_t i = 0; i < legs.size(); ++i) {
            const auto state = movers[i].state;
            if (i == index || state == StallMoverState::absent || state == StallMoverState::standing)
                continue;
            const auto &other = legs[i].body;
            const auto offset = other.position - mover.body.position;
            const float apart = length(offset);
            if (apart <= 0 ||
                apart > settings.pushing_reach * length(mover.body.half_extents + other.half_extents))
                continue;
            const auto direction = offset / apart;
            if (dot(direction, mover.heading) > settings.ahead_cosine &&
                dot(other.velocity, -direction) > settings.stop_speed)
                return {now, StallReason::pushed_back, i};
        }
    return {now,
            reversals >= settings.oscillation_reversals ? StallReason::oscillating : StallReason::unknown,
            std::nullopt};
}
} // namespace

void StallDetector::Track::observe_position(Vec2 position, float excursion) {
    const auto delta = position - anchor;
    const float excursion_squared = excursion * excursion;
    if (axis == Vec2{}) {
        const float squared = dot(delta, delta);
        if (squared >= excursion_squared) {
            forward_peak = std::sqrt(squared);
            axis = delta / forward_peak;
        }
        return;
    }
    const float forward = dot(delta, axis);
    forward_peak = std::max(forward_peak, forward);
    // Measure backtracking from the turning point, including motion that straddles the
    // anchor without getting far from it on either side.
    if (forward_peak - forward >= excursion) {
        ++reversals;
        anchor = position;
        axis = -axis;
        forward_peak = 0;
        return;
    }
    // Keep the sideways reference independent of small advances along the axis, so a
    // sustained sideways excursion can set a new axis.
    const auto sideways = delta - axis * forward;
    if (dot(sideways, sideways) >= excursion_squared) {
        anchor = position;
        axis = normalized(sideways);
        forward_peak = 0;
    }
}

std::vector<StallEvent> StallDetector::observe(std::span<const StallMover> movers,
                                               const StallObservation &settings, std::uint64_t now) {
    validate(settings);
    std::vector<Leg> legs;
    legs.reserve(movers.size());
    for (const auto &mover : movers) {
        kinetra::validate(mover.body);
        if (!std::isfinite(mover.target.x) || !std::isfinite(mover.target.y) ||
            !std::isfinite(mover.destination.x) || !std::isfinite(mover.destination.y))
            throw std::invalid_argument("Stall movers require a finite target and destination");
        if (mover.state != StallMoverState::observed && mover.state != StallMoverState::busy &&
            mover.state != StallMoverState::standing && mover.state != StallMoverState::absent)
            throw std::invalid_argument("Stall movers require a valid state");
        const auto body = footprint(mover.body);
        legs.push_back({body, mover.target, normalized(mover.target - body.position)});
    }
    tracks_.resize(std::max(tracks_.size(), movers.size()));
    std::vector<StallEvent> events;
    for (std::size_t i = 0; i < movers.size(); ++i) {
        const auto &mover = movers[i];
        auto &track = tracks_[i];
        const bool watching = mover.state == StallMoverState::observed && mover.no_progress > 0;
        // A caller can restart its progress count and count the next step at once, with the
        // goal unchanged; a count that does not grow is a new observation.
        if (!watching || track.goal != mover.destination || mover.no_progress <= track.last_no_progress) {
            if (track.episode)
                events.push_back({StallEvent::Kind::ended, i, *track.episode});
            track = Track{};
            track.goal = mover.destination;
            track.anchor = mover.body.position;
        }
        track.last_no_progress = mover.no_progress;
        if (!watching || track.episode)
            continue;
        track.observe_position(mover.body.position, settings.excursion);
        if (++track.observations >= settings.threshold_steps) {
            track.episode = diagnose(legs, movers, i, track.reversals, settings, now);
            events.push_back({StallEvent::Kind::started, i, *track.episode});
        }
    }
    return events;
}

std::optional<StallDiagnosis> StallDetector::episode(std::size_t mover) const {
    if (mover < tracks_.size())
        return tracks_[mover].episode;
    return std::nullopt;
}
} // namespace kinetra
