#include <kinetra/simulation/passing.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace kinetra {
namespace {
/// Headings at or below this cosine count as oncoming.
constexpr float oncoming_cosine = -0.8f;
/// A neighbor traveling against the lane faster than this share of the speed limit blocks it.
constexpr float oncoming_speed_fraction = 0.25f;

float support(Vec2 extent, Vec2 direction) {
    return std::abs(direction.x) * extent.x + std::abs(direction.y) * extent.y;
}

float braking_distance(float speed, float acceleration) {
    return speed * speed / acceleration;
}
} // namespace

std::optional<float> passing_conflict_time(const PassingApproach &first, const PassingApproach &second,
                                           float speed, float horizon, float gap) {
    if (dot(first.heading, second.heading) > oncoming_cosine)
        return std::nullopt;
    const auto offset = second.position - first.position;
    const auto relative = (first.heading - second.heading) * speed;
    const float square = dot(relative, relative);
    const float closing = dot(offset, relative);
    if (square <= 0 || closing <= 0)
        return std::nullopt;
    const float time = closing / square;
    if (time > horizon)
        return std::nullopt;
    const auto closest = offset - relative * time;
    const auto extent = first.half_extents + second.half_extents + Vec2{gap, gap};
    if (std::abs(closest.x) < extent.x && std::abs(closest.y) < extent.y)
        return time;
    return std::nullopt;
}

std::array<std::optional<PassingLanes>, 6>
passing_lane_candidates(const PassingApproach &first, const PassingApproach &second, PassingMargins margins) {
    std::array<std::optional<PassingLanes>, 6> candidates{};
    const auto axis = normalized(first.heading - second.heading);
    if (axis == Vec2{})
        return candidates;
    const float separation = dot(second.position - first.position, axis);
    const float along = support(first.half_extents, axis) + support(second.half_extents, axis);
    // The lane changes stay longitudinally separated even if one participant arrives first.
    if (separation < 2 * along + margins.formation_room)
        return candidates;

    const auto midpoint = (first.position + second.position) / 2;
    std::size_t index = 0;
    for (float hand : {-1.f, 1.f}) {
        const auto lateral = perpendicular(axis) * hand;
        const float step =
            std::max(support(first.half_extents, lateral), support(second.half_extents, lateral)) +
            margins.gap;
        for (float shift : {0.f, step, -step}) {
            const auto lane_for = [&](const PassingApproach &approach, float sign) {
                const float lane = sign * (support(approach.half_extents, lateral) + margins.gap);
                return PassingLane{approach.position + axis * (sign * along / 2) +
                                       lateral * (dot(midpoint - approach.position, lateral) + lane + shift),
                                   midpoint + axis * (sign * (along + margins.exit_offset)) +
                                       lateral * (lane + shift)};
            };
            const PassingLanes lanes{lane_for(first, 1), lane_for(second, -1)};
            // Leave room after the exit to resume the original trip.
            if (dot(first.destination - lanes[0].exit, axis) >= margins.gap &&
                dot(second.destination - lanes[1].exit, -axis) >= margins.gap)
                candidates[index] = lanes;
            ++index;
        }
    }
    return candidates;
}

PassingLaneGuard::PassingLaneGuard(const Body &participant, PassingLane lane, PassingPhase phase,
                                   Vec2 clearance, PassingPrediction prediction)
    : position_(participant.position), velocity_(participant.velocity),
      extent_(participant.half_extents + clearance), axis_(normalized(lane.exit - lane.entry)),
      prediction_(prediction) {
    if (!std::isfinite(prediction.speed) || prediction.speed <= 0 ||
        !std::isfinite(prediction.acceleration) || prediction.acceleration <= 0 || prediction.horizon < 0 ||
        prediction.sample_interval <= 0)
        throw std::invalid_argument(
            "Passing requires positive motion limits and a valid prediction interval");
    if (phase == PassingPhase::approach) {
        legs_ = {{{position_, lane.entry}, {lane.entry, lane.exit}}};
        leg_count_ = 2;
    } else {
        legs_[0] = {position_, lane.exit};
        leg_count_ = 1;
    }
    // Include enough room to stop before a predicted conflict.
    float reach = prediction.speed * float(prediction.horizon) +
                  braking_distance(prediction.speed, prediction.acceleration);
    for (std::size_t i = 0; i < leg_count_; ++i) {
        const auto leg = legs_[i];
        const auto travel = leg.end - leg.start;
        const float length = kinetra::length(travel);
        if (length >= reach) {
            const auto target = length > 0 ? leg.start + travel * (reach / length) : leg.start;
            near_[near_count_++] = {leg.start, target};
            break;
        }
        near_[near_count_++] = leg;
        reach -= length;
    }
}

Aabb PassingLaneGuard::query_bounds(float margin) const {
    auto area = swept_box_bounds(legs_[0].start, legs_[0].end, extent_);
    for (std::size_t i = 1; i < leg_count_; ++i) {
        const auto box = swept_box_bounds(legs_[i].start, legs_[i].end, extent_);
        area.min = {std::min(area.min.x, box.min.x), std::min(area.min.y, box.min.y)};
        area.max = {std::max(area.max.x, box.max.x), std::max(area.max.y, box.max.y)};
    }
    return {area.min - Vec2{margin, margin}, area.max + Vec2{margin, margin}};
}

PassingObstruction PassingLaneGuard::inspect(const Body &neighbor, bool stationary) const {
    // The first leg often runs sideways. Judge traffic along the lane's entry-to-exit axis.
    if (dot(neighbor.position - position_, axis_) <= 0)
        return PassingObstruction::clear;
    const auto occupies = [&](std::span<const Segment> legs, const Body &body) {
        return std::any_of(legs.begin(), legs.end(), [&](const Segment &leg) {
            return swept_box_overlaps(leg.start, leg.end, extent_, body);
        });
    };
    if (stationary)
        return occupies(std::span(legs_).first(leg_count_), neighbor) ? PassingObstruction::blocking
                                                                      : PassingObstruction::clear;

    auto sample = neighbor;
    for (int time = 0;; time += std::min(prediction_.sample_interval, prediction_.horizon - time)) {
        sample.position = neighbor.position + neighbor.velocity * float(time);
        if (occupies(std::span(near_).first(near_count_), sample))
            return dot(neighbor.velocity, axis_) < -oncoming_speed_fraction * prediction_.speed
                       ? PassingObstruction::blocking
                       : PassingObstruction::crossing;
        if (time == prediction_.horizon)
            return PassingObstruction::clear;
    }
}

Vec2 PassingLaneGuard::braking_target() const {
    return position_ + normalized(velocity_) * braking_distance(length(velocity_), prediction_.acceleration);
}
} // namespace kinetra
