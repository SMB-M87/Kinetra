#include <kinetra/traffic.h>
#include <array>
#include <iostream>
#include <stdexcept>

using namespace kinetra;
namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main() {
    try {
        TrafficGate gate;
        const std::array<std::uint64_t, 3> requests{9, 3, 7};
        auto state = gate.update(requests, {});
        require(state.agent == 3 && state.waiting == 2, "Simultaneous requests were not ordered by stable ID");
        const std::array<std::uint64_t, 4> newcomers{1, 7, 9, 3};
        state = gate.update(newcomers, {});
        require(state.agent == 3 && state.waiting == 3, "New arrival preempted an existing grant");
        const std::array<std::uint64_t, 3> remaining{1, 7, 9};
        const std::array<std::uint64_t, 1> occupied{3};
        state = gate.update(remaining, occupied);
        require(state.agent == 3 && state.waiting == 3, "Request cancellation released physical occupancy");
        state = gate.update(remaining, {});
        require(state.agent == 7, "Later low ID bypassed an older waiting request");
        const std::array<std::uint64_t, 2> last{1, 9};
        require(gate.update(last, {}).agent == 9, "Cancellation failed to release a grant");
        const std::array<std::uint64_t, 2> conflict{9, 7};
        state = gate.update(last, conflict);
        require(!state.agent && state.occupied_conflict && state.waiting == 2,
                "Multiple occupants admitted new traffic");
        state = gate.update(last, std::span<const std::uint64_t>(conflict).subspan(1));
        require(state.agent == 7 && state.waiting == 2, "Remaining occupant did not protect its exit");
        const std::array<std::uint64_t, 2> duplicate{1, 1};
        bool rejected = false;
        try { gate.update(duplicate, {}); }
        catch (const std::invalid_argument &) { rejected = true; }
        require(rejected, "Duplicate requests accepted");
        rejected = false;
        try { gate.update(last, duplicate); }
        catch (const std::invalid_argument &) { rejected = true; }
        require(rejected, "Duplicate occupancy accepted");
        require(gate.update(last, {}).agent == 9, "Invalid snapshot mutated FIFO state");
        gate.clear();
        require(gate.update(last, {}).agent == 1, "Clear retained a stale ticket");
        require(!gate.update({}, {}).agent, "Empty snapshots retained an admission");
        std::cout << "Traffic gate contracts passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
