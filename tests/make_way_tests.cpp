#include <doctest/doctest.h>

#include <kinetra/simulation/make_way.h>

#include <array>
#include <stdexcept>

using namespace kinetra;

namespace {
constexpr Vec2 goal{20, 0};
constexpr Vec2 home{10, 0};
constexpr Vec2 refuge{10, 2};
constexpr MakeWaySettings settings{10, 3, 2, 0.1f};

struct World : MakeWayWorld {
    std::array<MakeWayMover, 2> movers{{
        {{5, 0}, goal, {}, MakeWayMotion::moving, false},
        {home, home, home, MakeWayMotion::idle, false},
    }};
    std::optional<std::uint64_t> episode{1};
    std::vector<MakeWayEvent> events;
    bool alternate{}, may_yield{true}, can_find_refuge{true}, can_return{true};
    bool protecting{true}, obstructed{}, forbidden{};
    unsigned assignments{}, returns{};

    MakeWayMover mover(std::size_t index) const override { return movers.at(index); }

    std::optional<std::uint64_t> stall(std::size_t beneficiary) const override {
        return beneficiary == 0 ? episode : std::nullopt;
    }

    std::optional<MakeWayBlocker> blocker(std::size_t) const override { return MakeWayBlocker{1, may_yield}; }

    std::vector<Vec2> refuges(std::size_t) const override {
        return {{10, 4}, {10, 3}, refuge, {10, -2}, {10, 1}};
    }

    std::vector<Vec2> route(std::size_t, Vec2 target, MakeWayRoute purpose,
                            std::optional<MakeWayRelocation> relocated) const override {
        bool clear;
        if (purpose == MakeWayRoute::yielding)
            clear = target == home ? can_return : can_find_refuge && target != Vec2{10, 3};
        else if (relocated)
            clear = relocated->mover == 1 && relocated->position != Vec2{10, 1};
        else
            clear = alternate || movers[1].position == refuge;
        return clear ? std::vector{target} : std::vector<Vec2>{};
    }

    bool needs_home(std::size_t, std::size_t) const override { return protecting; }
    bool route_forbidden(std::size_t) const override { return forbidden; }
    bool route_obstructed(std::size_t) const override { return obstructed; }

    void assign(std::size_t index, std::vector<Vec2> path, MakeWayRoute purpose) override {
        auto &m = movers.at(index);
        m.destination = path.back();
        m.motion = MakeWayMotion::moving;
        m.yielding = purpose == MakeWayRoute::yielding;
        ++assignments;
    }

    void brake(std::size_t index) override {
        assign(index, {movers.at(index).position}, MakeWayRoute::yielding);
    }

    void returned(std::size_t index) override {
        movers.at(index).yielding = false;
        ++returns;
    }

    void event(MakeWayEvent value) override { events.push_back(value); }

    void arrive(std::size_t index) {
        auto &m = movers.at(index);
        m.position = m.destination;
        m.motion = MakeWayMotion::idle;
    }
};
} // namespace

TEST_CASE("make way tries the beneficiary's route before asking a blocker to yield") {
    MakeWay make_way;
    World world;
    world.alternate = true;
    world.may_yield = false;
    make_way.step(world, 2, 1, true, settings);
    CHECK(world.assignments == 1);
    CHECK_FALSE(world.movers[1].yielding);
    CHECK(world.events.empty());
    make_way.step(world, 2, 20, true, settings);
    CHECK(world.assignments == 1);
}

TEST_CASE("make way takes the nearest checked refuge and returns when home is clear") {
    MakeWay make_way;
    World world;
    make_way.step(world, 2, 1, true, settings);
    REQUIRE(world.events.size() == 1);
    CHECK(world.events.front().kind == MakeWayEvent::Kind::started);
    CHECK((world.events.front().beneficiary == 0 && world.events.front().blocker == 1));
    CHECK(world.movers[1].destination == refuge);

    world.arrive(1);
    make_way.step(world, 2, 2, true, settings);
    CHECK(world.assignments == 2);
    CHECK(world.movers[0].destination == goal);
    world.protecting = false;
    world.can_return = false;
    make_way.step(world, 2, 3, true, settings);
    REQUIRE(world.events.size() == 2);
    CHECK(world.events.back().outcome == MakeWayEnd::cleared);
    CHECK(world.movers[1].destination == refuge);

    world.can_return = true;
    make_way.step(world, 2, 4, true, settings);
    CHECK(world.movers[1].destination == home);
    world.arrive(1);
    make_way.step(world, 2, 5, true, settings);
    CHECK(world.returns == 1);
    CHECK_FALSE(world.movers[1].yielding);
    CHECK(world.events.size() == 2);
}

TEST_CASE("make way limits searches per episode and resets the limit for a new episode") {
    MakeWay make_way;
    World world;
    world.can_find_refuge = false;
    for (std::uint64_t now = 1; now <= 6; ++now) {
        make_way.step(world, 2, now, true, settings);
        REQUIRE(world.events.size() == (now < 4 ? 1u : 2u));
        CHECK(world.events.back().kind == MakeWayEvent::Kind::unavailable);
    }
    CHECK(world.assignments == 0);
    world.episode = 7;
    world.can_find_refuge = true;
    make_way.step(world, 2, 7, true, settings);
    CHECK(world.events.back().kind == MakeWayEvent::Kind::started);
}

TEST_CASE("a make way timeout can return while home is still needed") {
    MakeWay make_way;
    World world;
    make_way.step(world, 2, 1, true, settings);
    world.arrive(1);
    make_way.step(world, 2, 2, true, settings);
    make_way.step(world, 2, 10, true, settings);
    REQUIRE(world.events.size() == 1);
    make_way.step(world, 2, 11, true, settings);
    REQUIRE(world.events.size() == 2);
    CHECK(world.events.back().outcome == MakeWayEnd::timed_out);
    CHECK(world.movers[1].destination == home);
}

TEST_CASE("disabling, changing a goal, or holding a mover cancels make way once") {
    MakeWay make_way;
    World world;
    bool enabled = true;
    make_way.step(world, 2, 1, enabled, settings);
    world.movers[1].position = {10, 1};
    SUBCASE("disable the policy") {
        enabled = false;
    }
    SUBCASE("change the beneficiary goal") {
        world.movers[0].destination = {0, 0};
    }
    SUBCASE("hold the beneficiary") {
        world.movers[0].motion = MakeWayMotion::held;
    }
    SUBCASE("hold the blocker") {
        world.movers[1].motion = MakeWayMotion::held;
    }
    make_way.step(world, 2, 2, enabled, settings);
    REQUIRE(world.events.size() == 2);
    CHECK(world.events.back().outcome == MakeWayEnd::cancelled);
    if (world.movers[1].motion == MakeWayMotion::held) {
        CHECK(world.movers[1].destination == refuge);
        world.movers[1].motion = MakeWayMotion::moving;
    }
    make_way.step(world, 2, 3, enabled, settings);
    CHECK(world.events.size() == 2);
    CHECK(world.movers[1].destination == home);
}

TEST_CASE("make way stops at a live obstruction and checks the return again") {
    MakeWay make_way;
    World world;
    make_way.step(world, 2, 1, true, settings);
    world.movers[1].position = {10, 1};
    world.forbidden = true;
    world.can_return = false;
    make_way.step(world, 2, 2, true, settings);
    REQUIRE(world.events.size() == 2);
    CHECK(world.events.back().outcome == MakeWayEnd::obstructed);
    CHECK(world.movers[1].destination == world.movers[1].position);
    world.forbidden = false;
    world.can_return = true;
    make_way.step(world, 2, 3, true, settings);
    CHECK(world.movers[1].destination == home);
    CHECK(world.events.size() == 2);
}

TEST_CASE("make way cancels a removed blocker and an invalid call leaves the trip active") {
    MakeWay make_way;
    World world;
    make_way.step(world, 2, 1, true, settings);
    auto invalid = settings;
    invalid.max_searches = 0;
    CHECK_THROWS_AS(make_way.step(world, 1, 2, true, invalid), std::invalid_argument);
    CHECK(world.events.size() == 1);
    make_way.step(world, 1, 2, true, settings);
    REQUIRE(world.events.size() == 2);
    CHECK(world.events.back().outcome == MakeWayEnd::cancelled);
    CHECK(world.returns == 0);
    make_way.step(world, 1, 3, true, settings);
    CHECK(world.events.size() == 2);
}
