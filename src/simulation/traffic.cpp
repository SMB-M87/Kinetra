#include <algorithm>
#include <kinetra/traffic.h>
#include <stdexcept>

namespace kinetra {
namespace {
std::vector<std::uint64_t> ordered_ids(std::span<const std::uint64_t> ids) {
    std::vector<std::uint64_t> result(ids.begin(), ids.end());
    std::sort(result.begin(), result.end());
    if (std::adjacent_find(result.begin(), result.end()) != result.end())
        throw std::invalid_argument("Traffic snapshots require unique agent IDs");
    return result;
}
} // namespace
TrafficAdmission TrafficGate::update(std::span<const std::uint64_t> requests,
                                     std::span<const std::uint64_t> occupants) {
    return update(requests, occupants, {});
}
TrafficAdmission TrafficGate::update(std::span<const std::uint64_t> requests,
                                     std::span<const std::uint64_t> occupants,
                                     std::span<const std::uint64_t> ineligible) {
    const auto requesting = ordered_ids(requests);
    const auto occupying = ordered_ids(occupants);
    const auto blocked = ordered_ids(ineligible);
    auto requested = [&](std::uint64_t id) {
        return std::binary_search(requesting.begin(), requesting.end(), id);
    };
    for (auto id : blocked)
        if (!requested(id))
            throw std::invalid_argument("Ineligible traffic IDs must belong to the request snapshot");
    std::erase_if(queue_, [&](auto id) { return !requested(id); });
    for (auto id : requesting)
        if (std::find(queue_.begin(), queue_.end(), id) == queue_.end())
            queue_.push_back(id);
    if (occupying.size() > 1)
        admitted_.reset();
    else if (!occupying.empty())
        admitted_ = occupying.front();
    else if (!admitted_ || !requested(*admitted_)) {
        const auto ready = std::find_if(queue_.begin(), queue_.end(), [&](auto id) {
            return !std::binary_search(blocked.begin(), blocked.end(), id);
        });
        admitted_ = ready == queue_.end() ? std::nullopt : std::optional(*ready);
    }
    const bool admitted_request = admitted_ && requested(*admitted_);
    return {admitted_, queue_.size() - std::size_t(admitted_request), occupying.size() > 1};
}
void TrafficGate::clear() {
    queue_.clear();
    admitted_.reset();
}
} // namespace kinetra
