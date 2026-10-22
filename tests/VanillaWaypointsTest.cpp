#include "features/VanillaWaypointsState.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Id = std::int64_t;

void require(bool condition, char const* expression, int line) {
    if (!condition) throw std::runtime_error("line " + std::to_string(line) + ": " + expression);
}

#define REQUIRE(...) require((__VA_ARGS__), #__VA_ARGS__, __LINE__)

void emptyPlayerListNeedsNoWork() {
    std::vector<Id> players;
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({}, players, 1, [](Id) {
        throw std::runtime_error("empty player list queried a live actor");
        return true;
    }));
}

void selfNeedsNoWaypoint() {
    std::vector<Id> players{7, 7};
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({}, players, 7, [](Id) {
        throw std::runtime_error("self queried a live actor");
        return true;
    }));
}

void trackedPlayersDoNotResolveActors() {
    std::vector<Id> players{1, 2, 3, 4};
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({4, 2, 3}, players, 1, [](Id) {
        throw std::runtime_error("tracked player queried a live actor");
        return true;
    }));
}

void expiredMissingPlayersNeedNoWork() {
    std::vector<Id> players{1, 2, 3}, queried;
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({}, players, 1, [&](Id id) {
        queried.push_back(id);
        return false;
    }));
    REQUIRE(queried == std::vector<Id>{2, 3});
}

void missingLivePlayerRequiresNativeUpdate() {
    std::vector<Id> players{1, 2, 3}, queried;
    REQUIRE(lo::detail::hasMissingLiveWaypoint({2}, players, 1, [&](Id id) {
        queried.push_back(id);
        return true;
    }));
    REQUIRE(queried == std::vector<Id>{3});
}

void duplicateIdsPreserveMembership() {
    std::vector<Id> players{1, 2, 2, 3, 3};
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({3, 2, 3, 2, 3}, players, 1, [](Id) {
        throw std::runtime_error("duplicate tracked ID lost membership");
        return true;
    }));
    REQUIRE(lo::detail::hasMissingLiveWaypoint({2, 2}, players, 1, [](Id id) { return id == 3; }));
}

void eachCallObservesJoinsAndLeaves() {
    std::vector<Id> players{1, 2};
    auto            isLive = [](Id id) { return id == 2 || id == 3; };
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({2}, players, 1, isLive));
    players.push_back(3);
    REQUIRE(lo::detail::hasMissingLiveWaypoint({2}, players, 1, isLive));
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({2, 3}, players, 1, isLive));
    players = {1, 2};
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({2, 3}, players, 1, isLive));
    players = {1, 4};
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({2, 3}, players, 1, isLive));
    REQUIRE(lo::detail::hasMissingLiveWaypoint({}, players, 1, [](Id) { return true; }));
}

void bulkMembershipAndLateMissingActor() {
    auto   players = std::views::iota(Id{0}, Id{10001});
    auto   tracked = std::ranges::to<std::vector<Id>>(std::views::iota(Id{1}, Id{10001}) | std::views::reverse);
    size_t queries{};
    auto   live = [&](Id) {
        ++queries;
        return true;
    };
    REQUIRE(!lo::detail::hasMissingLiveWaypoint(tracked, players, 0, live));
    REQUIRE(queries == 0);
    tracked.pop_back();
    REQUIRE(lo::detail::hasMissingLiveWaypoint(std::move(tracked), players, 0, live));
    REQUIRE(queries == 1);
}

void livePredicateShortCircuitsAtFirstMissingActor() {
    std::vector<Id> players{1, 2, 3, 4, 5}, queried;
    REQUIRE(lo::detail::hasMissingLiveWaypoint({2}, players, 1, [&](Id id) {
        queried.push_back(id);
        return id == 4;
    }));
    REQUIRE(queried == std::vector<Id>{3, 4});
}

void signedIdBoundariesUseExactMembership() {
    auto const      lowest  = std::numeric_limits<Id>::min();
    auto const      highest = std::numeric_limits<Id>::max();
    std::vector<Id> players{lowest, -1, 0, highest};
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({highest, -1, lowest}, players, 0, [](Id) { return true; }));
    REQUIRE(lo::detail::hasMissingLiveWaypoint({highest, lowest}, players, 0, [](Id id) { return id == -1; }));
}

struct SimulatedFailure {};

void actorLookupFailureLeavesNoCachedState() {
    std::vector<Id> players{1, 2};
    bool            threw{};
    try {
        (void)lo::detail::hasMissingLiveWaypoint({}, players, 1, [](Id) -> bool { throw SimulatedFailure{}; });
    } catch (SimulatedFailure const&) {
        threw = true;
    }
    REQUIRE(threw);
    REQUIRE(!lo::detail::hasMissingLiveWaypoint({2}, players, 1, [](Id) { return true; }));
}

} // namespace

int main() {
    std::pair<char const*, void (*)()> const tests[]{
        {"empty player list",            emptyPlayerListNeedsNoWork                   },
        {"self excluded",                selfNeedsNoWaypoint                          },
        {"tracked actors bypass lookup", trackedPlayersDoNotResolveActors             },
        {"expired actors excluded",      expiredMissingPlayersNeedNoWork              },
        {"missing live actor",           missingLivePlayerRequiresNativeUpdate        },
        {"duplicate IDs",                duplicateIdsPreserveMembership               },
        {"join and leave between calls", eachCallObservesJoinsAndLeaves               },
        {"bulk IDs",                     bulkMembershipAndLateMissingActor            },
        {"predicate short circuit",      livePredicateShortCircuitsAtFirstMissingActor},
        {"signed ID boundaries",         signedIdBoundariesUseExactMembership         },
        {"lookup failure recovery",      actorLookupFailureLeavesNoCachedState        },
    };
    size_t failed{};
    for (auto const& [name, run] : tests) {
        try {
            run();
            std::cout << "PASS " << name << '\n';
        } catch (std::exception const& error) {
            ++failed;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    }
    std::cout << std::size(tests) - failed << '/' << std::size(tests) << " tests passed\n";
    return failed == 0 ? 0 : 1;
}
