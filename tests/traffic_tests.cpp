// Contract tests for kinetra::TrafficGate (include/kinetra/simulation/traffic.h).
// This file is the doctest pattern for converting the other suites: one TEST_CASE
// per behavior, named after the expected outcome.
#include <kinetra/simulation/traffic.h>

#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

using kinetra::TrafficAdmission;
using kinetra::TrafficGate;

namespace {
using Ids = std::vector<std::uint64_t>;

/// One update of the shared admission history: requesting IDs and physical occupants.
struct Step {
    Ids requests;
    Ids occupants;
};

/// History in which agent 3 is admitted first, newcomers queue behind older requests,
/// and occupancy and cancellation interact. Cases replay a prefix of it, so each case
/// checks one step in isolation while starting from the same state as the original sequence.
const std::array<Step, 7> history{{
    {{9, 3, 7}, {}},    // simultaneous requests
    {{1, 7, 9, 3}, {}}, // newcomer 1 arrives
    {{1, 7, 9}, {3}},   // 3 cancels its request but still occupies the zone
    {{1, 7, 9}, {}},    // 3 has left
    {{1, 9}, {}},       // 7 cancels while admitted
    {{1, 9}, {9, 7}},   // two occupants at once
    {{1, 9}, {7}},      // 9 has left, 7 remains inside
}};

/// Feeds the first `count` history steps to `gate` and returns the last admission.
TrafficAdmission replay(TrafficGate &gate, std::size_t count) {
    TrafficAdmission state{};
    for (std::size_t i = 0; i < count; ++i)
        state = gate.update(history[i].requests, history[i].occupants);
    return state;
}

const Ids simultaneous{9, 3, 7};
const Ids duplicate_one{1, 1};
} // namespace

TEST_CASE("simultaneous requests are admitted in stable ID order") {
    TrafficGate gate;
    const auto state = replay(gate, 1);
    CHECK(state.agent == 3u);
    CHECK(state.waiting == 2u);
}

TEST_CASE("a new arrival does not preempt an existing grant") {
    TrafficGate gate;
    const auto state = replay(gate, 2);
    CHECK(state.agent == 3u);
    CHECK(state.waiting == 3u);
}

TEST_CASE("physical occupancy keeps the grant after the request is cancelled") {
    TrafficGate gate;
    const auto state = replay(gate, 3);
    CHECK(state.agent == 3u);
    CHECK(state.waiting == 3u);
}

TEST_CASE("the oldest waiting request is admitted next, not the lowest ID") {
    TrafficGate gate;
    CHECK(replay(gate, 4).agent == 7u);
}

TEST_CASE("cancelling a granted request releases the grant") {
    TrafficGate gate;
    CHECK(replay(gate, 5).agent == 9u);
}

TEST_CASE("multiple occupants block new admission") {
    TrafficGate gate;
    const auto state = replay(gate, 6);
    CHECK_FALSE(state.agent.has_value());
    CHECK(state.occupied_conflict);
    CHECK(state.waiting == 2u);
}

TEST_CASE("a remaining occupant keeps its grant so it can exit") {
    TrafficGate gate;
    const auto state = replay(gate, 7);
    CHECK(state.agent == 7u);
    CHECK(state.waiting == 2u);
}

TEST_CASE("invalid snapshots are rejected without changing FIFO state") {
    TrafficGate gate;
    replay(gate, history.size());
    const Ids last{1, 9};
    CHECK_THROWS_AS(gate.update(duplicate_one, {}), std::invalid_argument);
    CHECK_THROWS_AS(gate.update(last, duplicate_one), std::invalid_argument);
    CHECK(gate.update(last, {}).agent == 9u);
}

TEST_CASE("clear forgets tickets and empty snapshots admit nobody") {
    TrafficGate gate;
    replay(gate, history.size());
    const Ids last{1, 9};
    gate.update(last, {});
    gate.clear();
    CHECK(gate.update(last, {}).agent == 1u);
    CHECK_FALSE(gate.update({}, {}).agent.has_value());
}

TEST_CASE("an ineligible head lets the next eligible request in without losing its ticket") {
    TrafficGate gate;
    replay(gate, history.size());
    gate.clear();
    const Ids blocked_head{3};
    auto state = gate.update(simultaneous, {}, blocked_head);
    CHECK(state.agent == 7u);
    CHECK(state.waiting == 2u);

    // Temporary ineligibility never revokes a grant that may already be approaching.
    state = gate.update(simultaneous, {}, simultaneous);
    CHECK(state.agent == 7u);

    // 7 left; 3 recovered and keeps its original, older ticket.
    const Ids survivors{3, 9};
    CHECK(gate.update(survivors, {}).agent == 3u);
}

TEST_CASE("an all-ineligible queue admits nobody") {
    TrafficGate gate;
    CHECK_FALSE(gate.update(simultaneous, {}, simultaneous).agent.has_value());
}

TEST_CASE("invalid eligibility lists are rejected without changing queue state") {
    TrafficGate gate;
    gate.update(simultaneous, {}, simultaneous);
    const Ids unknown{42};
    CHECK_THROWS_AS(gate.update(simultaneous, {}, unknown), std::invalid_argument);
    CHECK_THROWS_AS(gate.update(simultaneous, {}, duplicate_one), std::invalid_argument);
    CHECK(gate.update(simultaneous, {}).agent == 3u);
}
