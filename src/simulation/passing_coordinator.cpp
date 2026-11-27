#include <kinetra/simulation/passing_coordinator.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace kinetra {
namespace {
/// Share of the speed limit per step by which the summed distance to the targets must
/// shrink to count as progress.
constexpr float progress_speed_fraction = 0.05f;

bool nonnegative(float value) {
    return std::isfinite(value) && value >= 0;
}

bool positive(float value) {
    return std::isfinite(value) && value > 0;
}

void validate(const PassingCoordination &settings) {
    const auto &margins = settings.margins;
    const auto &prediction = settings.prediction;
    if (!nonnegative(margins.gap) || !nonnegative(margins.exit_offset) ||
        !nonnegative(margins.formation_room) || !positive(prediction.speed) ||
        !positive(prediction.acceleration) || prediction.horizon < 0 || prediction.sample_interval <= 0 ||
        !nonnegative(settings.clearance.x) || !nonnegative(settings.clearance.y) ||
        !positive(settings.arrival_distance) || !positive(settings.stop_speed) || settings.timeout_steps == 0)
        throw std::invalid_argument("Passing coordination requires finite margins and clearance, positive "
                                    "limits, arrival values and timeout, and a valid prediction interval");
}

/// An axis-aligned box centered on the body's position that covers its bounds, so the
/// targets the caller steers the body's position to are the footprint's targets.
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

/// How far ahead plans are formed: the horizon in steps, at least long enough for two lane
/// changes before the movers meet, and the distance around a mover within which an oncoming
/// one can conflict within it. `largest` is the length of the largest half extents.
struct FormationReach {
    float horizon, range;
};

FormationReach formation_reach(const PassingCoordination &settings, float largest) {
    const auto &prediction = settings.prediction;
    const float horizon = std::max(float(prediction.horizon),
                                   (4 * largest + settings.margins.formation_room) / prediction.speed +
                                       2 * prediction.speed / prediction.acceleration);
    return {horizon, 2 * prediction.speed * horizon + 2 * largest};
}

/// Target of one participant in its current phase, ignoring a wait.
Vec2 lane_target(const PassingLane &lane, bool crossing) {
    return crossing ? lane.exit : lane.entry;
}
} // namespace

std::vector<PassingEvent> PassingCoordinator::step(std::span<const PassingMover> movers,
                                                   const PassingCoordination &settings,
                                                   const PassingLaneCheck &lanes_clear) {
    validate(settings);
    std::vector<Body> footprints;
    footprints.reserve(movers.size());
    std::vector<Aabb> boxes;
    boxes.reserve(movers.size());
    std::vector<std::size_t> ids;
    Vec2 largest_half{};
    for (std::size_t id = 0; id < movers.size(); ++id) {
        const auto &mover = movers[id];
        kinetra::validate(mover.body);
        if (!finite(mover.heading) || !finite(mover.destination))
            throw std::invalid_argument("Passing movers require a finite heading and destination");
        footprints.push_back(footprint(mover.body));
        if (!mover.present)
            continue;
        const auto &box = footprints.back();
        boxes.push_back({box.position - box.half_extents, box.position + box.half_extents});
        ids.push_back(id);
        largest_half = {std::max(largest_half.x, box.half_extents.x),
                        std::max(largest_half.y, box.half_extents.y)};
    }
    // The widest query is the formation range around a mover; the guard's queries cover a
    // plan's legs, which lie within it. A cell near the largest footprint, or an eighth of
    // that range when it is longer, keeps every query to a bounded number of cells for any
    // scale and tuning: a range set by slow braking (speed²/acceleration) would otherwise
    // span more cells than the grid allows.
    const float cell = std::max(2 * std::max(largest_half.x, largest_half.y),
                                formation_reach(settings, length(largest_half)).range / 8);
    Neighbors index{SpatialGrid(cell), std::move(ids)};
    index.grid.rebuild(boxes);
    retry_.resize(std::max(retry_.size(), movers.size()));

    std::vector<PassingEvent> events;
    advance(footprints, movers, settings, index, largest_half, events);
    form(footprints, movers, settings, lanes_clear, index, largest_half, events);
    ++step_;
    return events;
}

std::vector<std::size_t> PassingCoordinator::Neighbors::query(Aabb area) const {
    auto found = grid.query(area);
    // The grid returns sorted positions, and ids is ascending, so the result stays sorted.
    for (auto &position : found)
        position = ids[position];
    return found;
}

std::vector<PassingEvent> PassingCoordinator::cancel(std::size_t mover) {
    std::vector<PassingEvent> events;
    for (std::size_t i = 0; i < plans_.size();) {
        if (plans_[i].movers[0] == mover || plans_[i].movers[1] == mover)
            end(i, PassingOutcome::cancelled, 0, events);
        else
            ++i;
    }
    return events;
}

std::vector<PassingEvent> PassingCoordinator::clear() {
    std::vector<PassingEvent> events;
    while (!plans_.empty())
        end(0, PassingOutcome::cancelled, 0, events);
    return events;
}

std::optional<PassingTarget> PassingCoordinator::target(std::size_t mover) const {
    for (const auto &plan : plans_)
        for (std::size_t side = 0; side < 2; ++side)
            if (plan.movers[side] == mover)
                return PassingTarget{plan.waiting[side] ? plan.hold[side]
                                                        : lane_target(plan.lanes[side], plan.crossing),
                                     plan.waiting[side]};
    return std::nullopt;
}

std::vector<PassingPlanState> PassingCoordinator::plans() const {
    std::vector<PassingPlanState> states;
    states.reserve(plans_.size());
    for (const auto &plan : plans_)
        states.push_back({plan.id, plan.movers, plan.lanes, plan.crossing, plan.waiting});
    return states;
}

void PassingCoordinator::end(std::size_t index, PassingOutcome outcome, std::uint64_t retry_steps,
                             std::vector<PassingEvent> &events) {
    const auto &plan = plans_[index];
    events.push_back({PassingEventKind::ended, plan.id, plan.movers, plan.lanes, outcome});
    // After a failed plan the movers wait before they try again, so they do not thrash.
    // After a completed or cancelled one they may pass the next oncoming mover at once:
    // waiting only lets it come too close for a plan.
    const bool failed = outcome != PassingOutcome::completed && outcome != PassingOutcome::cancelled;
    for (auto id : plan.movers)
        if (id < retry_.size())
            retry_[id] = failed ? step_ + retry_steps : step_;
    plans_.erase(plans_.begin() + std::ptrdiff_t(index));
}

void PassingCoordinator::advance(std::span<const Body> footprints, std::span<const PassingMover> movers,
                                 const PassingCoordination &settings, const Neighbors &index,
                                 Vec2 largest_half, std::vector<PassingEvent> &events) {
    // A neighbor ahead matters while its footprint can reach a remaining leg within the horizon.
    const float margin =
        2 * length(largest_half) + settings.prediction.speed * float(settings.prediction.horizon);
    for (std::size_t i = 0; i < plans_.size();) {
        auto &plan = plans_[i];
        // The first reason found ends the plan.
        std::optional<PassingOutcome> outcome;
        bool arrived = true;
        for (std::size_t side = 0; side < 2 && !outcome; ++side) {
            const auto id = plan.movers[side];
            if (id >= movers.size() || !movers[id].available || !movers[id].present) {
                outcome = PassingOutcome::unavailable;
                break;
            }
            const auto &body = footprints[id];
            arrived = arrived &&
                      distance(body.position, lane_target(plan.lanes[side], plan.crossing)) <
                          settings.arrival_distance &&
                      length(body.velocity) < settings.stop_speed;
        }
        if (!outcome)
            outcome = guard(plan, footprints, movers, settings, index, margin);
        if (!outcome && stalled(plan, footprints, settings))
            outcome = PassingOutcome::no_progress;
        if (!outcome && arrived && plan.crossing)
            outcome = PassingOutcome::completed;
        if (outcome) {
            end(i, *outcome, settings.retry_steps, events);
            continue;
        }
        if (arrived) {
            plan.crossing = true;
            plan.best_remaining = std::numeric_limits<float>::infinity();
            plan.progress_at = step_;
            events.push_back({PassingEventKind::crossing, plan.id, plan.movers, plan.lanes, std::nullopt});
        }
        ++i;
    }
}

bool PassingCoordinator::stalled(Plan &plan, std::span<const Body> footprints,
                                 const PassingCoordination &settings) {
    float remaining = 0;
    for (std::size_t side = 0; side < 2; ++side)
        remaining +=
            distance(footprints[plan.movers[side]].position, lane_target(plan.lanes[side], plan.crossing));
    if (remaining + settings.prediction.speed * progress_speed_fraction < plan.best_remaining) {
        plan.best_remaining = remaining;
        plan.progress_at = step_;
    }
    return step_ - plan.progress_at >= settings.timeout_steps;
}

bool PassingCoordinator::yields_to(const Plan &plan, std::size_t other) const {
    for (const auto &candidate : plans_)
        if (candidate.movers[0] == other || candidate.movers[1] == other)
            return candidate.id < plan.id;
    return true;
}

std::optional<PassingOutcome> PassingCoordinator::guard(Plan &plan, std::span<const Body> footprints,
                                                        std::span<const PassingMover> movers,
                                                        const PassingCoordination &settings,
                                                        const Neighbors &index, float margin) const {
    const auto phase = plan.crossing ? PassingPhase::traverse : PassingPhase::approach;
    std::optional<PassingOutcome> result;
    for (std::size_t side = 0; side < 2; ++side) {
        const auto &participant = footprints[plan.movers[side]];
        const PassingLaneGuard lane_guard(participant, plan.lanes[side], phase, settings.clearance,
                                          settings.prediction);
        bool intruded = false;
        for (auto other : index.query(lane_guard.query_bounds(margin))) {
            if (other == plan.movers[0] || other == plan.movers[1])
                continue;
            const bool stationary = movers[other].stationary;
            // A stationary mover is an obstruction regardless of passing priority.
            if (!stationary && !yields_to(plan, other))
                continue;
            const auto obstruction = lane_guard.inspect(footprints[other], stationary);
            if (obstruction == PassingObstruction::blocking)
                result = PassingOutcome::obstructed;
            if (obstruction == PassingObstruction::crossing)
                intruded = true;
        }
        if (intruded) {
            plan.intruded_at[side] = step_;
            if (!plan.waiting[side]) {
                plan.waiting[side] = true;
                plan.waiting_since[side] = step_;
                // Within the braking distance the lane ahead reserves, so the stop needs no
                // more than the acceleration limit and the participant does not overshoot.
                plan.hold[side] = lane_guard.braking_target();
            }
        } else if (plan.waiting[side] && step_ - plan.intruded_at[side] >= settings.resume_steps) {
            plan.waiting[side] = false;
        }
        if (plan.waiting[side] && step_ - plan.waiting_since[side] >= settings.timeout_steps)
            result = PassingOutcome::obstructed;
    }
    return result;
}

void PassingCoordinator::form(std::span<const Body> footprints, std::span<const PassingMover> movers,
                              const PassingCoordination &settings, const PassingLaneCheck &lanes_clear,
                              const Neighbors &index, Vec2 largest_half, std::vector<PassingEvent> &events) {
    std::vector<bool> claimed(movers.size());
    for (const auto &plan : plans_)
        for (auto id : plan.movers)
            claimed[id] = true;
    const auto &prediction = settings.prediction;
    const float largest = length(largest_half);
    // Look far enough ahead to fit two lane changes before the movers meet.
    const auto [horizon, range] = formation_reach(settings, largest);
    std::vector<PassingApproach> approaches;
    approaches.reserve(movers.size());
    for (std::size_t i = 0; i < movers.size(); ++i)
        approaches.push_back(
            {footprints[i].position, footprints[i].half_extents, movers[i].heading, movers[i].destination});
    const auto eligible = [&](std::size_t id) {
        const auto &mover = movers[id];
        return !claimed[id] && mover.present && mover.available && !mover.stationary &&
               mover.heading != Vec2{} && retry_[id] <= step_;
    };
    for (std::size_t a = 0; a < movers.size(); ++a) {
        if (!eligible(a))
            continue;
        const auto position = footprints[a].position;
        // Oncoming movers whose closest approach, at full speed, would bring them into contact,
        // nearest in time first.
        std::vector<std::pair<float, std::size_t>> candidates;
        for (auto b : index.query({position - Vec2{range, range}, position + Vec2{range, range}})) {
            if (b <= a || !eligible(b))
                continue;
            if (const auto time = passing_conflict_time(approaches[a], approaches[b], prediction.speed,
                                                        horizon, settings.margins.gap))
                candidates.push_back({*time, b});
        }
        std::sort(candidates.begin(), candidates.end());
        for (const auto &[time, b] : candidates) {
            bool formed = false;
            for (const auto &lanes :
                 passing_lane_candidates(approaches[a], approaches[b], settings.margins)) {
                if (!lanes)
                    continue;
                Plan plan;
                plan.movers = {a, b};
                plan.lanes = *lanes;
                plan.progress_at = step_;
                plan.best_remaining = std::numeric_limits<float>::infinity();
                if (!reserve(plan, footprints, settings, lanes_clear, index))
                    continue;
                plan.id = next_plan_++;
                events.push_back({PassingEventKind::formed, plan.id, plan.movers, plan.lanes, std::nullopt});
                plans_.push_back(std::move(plan));
                claimed[a] = claimed[b] = true;
                formed = true;
                break;
            }
            if (formed)
                break;
        }
    }
}

bool PassingCoordinator::reserve(Plan &plan, std::span<const Body> footprints,
                                 const PassingCoordination &settings, const PassingLaneCheck &lanes_clear,
                                 const Neighbors &index) const {
    // Each participant's two legs must stay clear of the caller's world, of third movers and
    // of the space other plans reserved; the swept areas are recorded in `plan.space`.
    for (std::size_t side = 0; side < 2; ++side) {
        const auto id = plan.movers[side];
        const auto half = footprints[id].half_extents + settings.clearance;
        Vec2 from = footprints[id].position;
        for (auto to : {plan.lanes[side].entry, plan.lanes[side].exit}) {
            if (lanes_clear && !lanes_clear(id, from, to, half))
                return false;
            const auto area = swept_box_bounds(from, to, half);
            for (auto other : index.query(area))
                if (other != plan.movers[0] && other != plan.movers[1] &&
                    swept_box_overlaps(from, to, half, footprints[other]))
                    return false;
            for (const auto &other : plans_)
                for (const auto &reserved : other.space)
                    if (reserved.overlaps(area))
                        return false;
            plan.space.push_back(area);
            from = to;
        }
    }
    return true;
}
} // namespace kinetra
