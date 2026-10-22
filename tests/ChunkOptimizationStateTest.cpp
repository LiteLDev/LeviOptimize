// SPDX-License-Identifier: GPL-3.0-or-later

#include "features/ChunkOptimizationState.h"

#include <array>
#include <barrier>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

void require(bool condition, char const* expression, int line) {
    if (!condition) {
        throw std::runtime_error("line " + std::to_string(line) + ": " + expression);
    }
}

#define REQUIRE(...) require((__VA_ARGS__), #__VA_ARGS__, __LINE__)

struct Policy {
    int id;
};
using Batch = lo::detail::InfluenceBatch<Policy>;

struct SimulatedFailure {};

void noActiveBatch() {
    Policy policy{1};
    REQUIRE(!Batch::defer(&policy));
}

void deduplicatesPolicies() {
    Policy               first{1}, second{2}, third{3};
    std::vector<Policy*> updated;
    Batch                batch;
    REQUIRE(Batch::defer(&first));
    REQUIRE(Batch::defer(&second));
    REQUIRE(Batch::defer(&first));
    REQUIRE(Batch::defer(&third));
    REQUIRE(Batch::defer(&second));
    REQUIRE(updated.empty());
    batch.finish([&](Policy* policy) { updated.push_back(policy); });
    REQUIRE(updated == std::vector<Policy*>{&first, &second, &third});
    batch.finish([&](Policy* policy) { updated.push_back(policy); });
    REQUIRE(updated.size() == 3);
    REQUIRE(!Batch::defer(&first));
}

void nestedBatchesFlushAtOuterBoundary() {
    Policy               first{1}, second{2}, third{3};
    std::vector<Policy*> updated;
    Batch                outer;
    REQUIRE(Batch::defer(&first));
    {
        Batch inner;
        REQUIRE(Batch::defer(&first));
        REQUIRE(Batch::defer(&second));
        inner.finish([&](Policy* policy) { updated.push_back(policy); });
        REQUIRE(updated.empty());
    }
    REQUIRE(Batch::defer(&third));
    outer.finish([&](Policy* policy) { updated.push_back(policy); });
    REQUIRE(updated == std::vector<Policy*>{&first, &second, &third});
}

void restoresContextBeforeCallbacks() {
    Policy               first{1}, second{2};
    std::vector<Policy*> updated;
    Batch                outer;
    REQUIRE(Batch::defer(&first));
    outer.finish([&](Policy* policy) {
        REQUIRE(!Batch::defer(policy));
        updated.push_back(policy);
        Batch callbackBatch;
        REQUIRE(Batch::defer(&second));
        callbackBatch.finish([&](Policy* nestedPolicy) {
            REQUIRE(!Batch::defer(nestedPolicy));
            updated.push_back(nestedPolicy);
        });
        REQUIRE(!Batch::defer(policy));
    });
    REQUIRE(updated == std::vector<Policy*>{&first, &second});
    REQUIRE(!Batch::defer(&first));
}

void exceptionDiscardsAbandonedBatch() {
    Policy               abandoned{1}, next{2};
    std::vector<Policy*> updated;
    try {
        Batch batch;
        REQUIRE(Batch::defer(&abandoned));
        throw SimulatedFailure{};
    } catch (SimulatedFailure const&) {}
    REQUIRE(!Batch::defer(&next));
    Batch batch;
    REQUIRE(Batch::defer(&next));
    batch.finish([&](Policy* policy) { updated.push_back(policy); });
    REQUIRE(updated == std::vector<Policy*>{&next});
}

void nestedExceptionDiscardsOnlyAbandonedUpdates() {
    Policy               retained{1}, abandoned{2}, afterCatch{3};
    std::vector<Policy*> updated;
    Batch                outer;
    REQUIRE(Batch::defer(&retained));
    try {
        Batch inner;
        REQUIRE(Batch::defer(&retained));
        REQUIRE(Batch::defer(&abandoned));
        throw SimulatedFailure{};
    } catch (SimulatedFailure const&) {}
    REQUIRE(Batch::defer(&afterCatch));
    outer.finish([&](Policy* policy) { updated.push_back(policy); });
    REQUIRE(updated == std::vector<Policy*>{&retained, &afterCatch});
}

void callbackExceptionRestoresContext() {
    Policy first{1}, second{2}, next{3};
    try {
        Batch batch;
        REQUIRE(Batch::defer(&first));
        REQUIRE(Batch::defer(&second));
        batch.finish([](Policy*) { throw SimulatedFailure{}; });
    } catch (SimulatedFailure const&) {}
    REQUIRE(!Batch::defer(&next));
    std::vector<Policy*> updated;
    Batch                batch;
    REQUIRE(Batch::defer(&next));
    batch.finish([&](Policy* policy) { updated.push_back(policy); });
    REQUIRE(updated == std::vector<Policy*>{&next});
}

void batchesAreThreadLocal() {
    std::array<Policy, 3> policies{
        {{1}, {2}, {3}}
    };
    struct ThreadResult {
        bool                 deferred{};
        bool                 callbackContextRestored = true;
        std::vector<Policy*> updated;
    };
    std::array<ThreadResult, 2> results;
    std::barrier                rendezvous{3};
    Batch                       mainBatch;
    REQUIRE(Batch::defer(&policies[2]));
    auto worker = [&](size_t index) {
        Batch batch;
        auto& result    = results[index];
        result.deferred = Batch::defer(&policies[index]);
        rendezvous.arrive_and_wait();
        rendezvous.arrive_and_wait();
        batch.finish([&](Policy* policy) {
            result.callbackContextRestored = result.callbackContextRestored && !Batch::defer(policy);
            result.updated.push_back(policy);
        });
    };
    std::thread first(worker, 0), second(worker, 1);
    rendezvous.arrive_and_wait();
    std::vector<Policy*> mainUpdated;
    mainBatch.finish([&](Policy* policy) { mainUpdated.push_back(policy); });
    rendezvous.arrive_and_wait();
    first.join();
    second.join();
    REQUIRE(mainUpdated == std::vector<Policy*>{&policies[2]});
    for (size_t index = 0; index < results.size(); ++index) {
        REQUIRE(results[index].deferred);
        REQUIRE(results[index].callbackContextRestored);
        REQUIRE(results[index].updated == std::vector<Policy*>{&policies[index]});
    }
    REQUIRE(!Batch::defer(&policies[2]));
}

// Count full key enumerations through the map interface, without inspecting the cache.
class ObservedMap {
    std::map<int, int> values;

public:
    mutable size_t enumerations{};

    explicit ObservedMap(std::initializer_list<int> keys) { replace(keys); }

    void replace(std::initializer_list<int> keys) {
        values.clear();
        for (int key : keys) values.emplace(key, key);
    }

    [[nodiscard]] bool   empty() const { return values.empty(); }
    [[nodiscard]] size_t size() const { return values.size(); }
    auto                 begin() const {
        ++enumerations;
        return values.begin();
    }
    auto end() const { return values.end(); }
};

void snapshotRefreshesAfterExactly400Picks() {
    lo::detail::ChunkSnapshot<int> snapshot;
    ObservedMap                    map{10, 20, 30};
    auto*                          key = snapshot.pick(map, 0);
    REQUIRE(key && *key == 10);
    REQUIRE(map.enumerations == 1);
    map.replace({40, 50, 60});
    for (int pick = 2; pick <= 400; ++pick) {
        key = snapshot.pick(map, 0);
        REQUIRE(key && *key == 10);
        REQUIRE(map.enumerations == 1);
    }
    key = snapshot.pick(map, 0);
    REQUIRE(key && *key == 40);
    REQUIRE(map.enumerations == 2);
    map.replace({70, 80, 90});
    for (int pick = 402; pick <= 800; ++pick) {
        key = snapshot.pick(map, 0);
        REQUIRE(key && *key == 40);
        REQUIRE(map.enumerations == 2);
    }
    key = snapshot.pick(map, 0);
    REQUIRE(key && *key == 70);
    REQUIRE(map.enumerations == 3);
}

void emptyAndInvalidPicksDoNotBuildSnapshots() {
    lo::detail::ChunkSnapshot<int> snapshot;
    ObservedMap                    map{};
    REQUIRE(!snapshot.pick(map, 0));
    REQUIRE(map.enumerations == 0);
    map.replace({1, 2, 3});
    REQUIRE(!snapshot.pick(map, map.size()));
    REQUIRE(!snapshot.pick(map, std::numeric_limits<size_t>::max()));
    REQUIRE(map.enumerations == 0);
    REQUIRE(snapshot.pick(map, 0));
    REQUIRE(map.enumerations == 1);
    map.replace({});
    for (int pick = 0; pick < 500; ++pick) REQUIRE(!snapshot.pick(map, 0));
    REQUIRE(map.enumerations == 1);
}

void mapGrowthAndShrinkStayWithinSnapshotBounds() {
    lo::detail::ChunkSnapshot<int> snapshot;
    ObservedMap                    map{10, 20, 30, 40, 50};
    REQUIRE(snapshot.pick(map, 0));
    map.replace({101, 102, 103, 104, 105, 106, 107, 108, 109, 110});
    for (size_t index = 0; index < map.size(); ++index) {
        auto* key = snapshot.pick(map, index);
        REQUIRE(key);
        REQUIRE(*key == 10 || *key == 20 || *key == 30 || *key == 40 || *key == 50);
    }
    map.replace({201});
    auto* key = snapshot.pick(map, 0);
    REQUIRE(key);
    REQUIRE(*key == 10 || *key == 20 || *key == 30 || *key == 40 || *key == 50);
    REQUIRE(!snapshot.pick(map, 1));
    REQUIRE(map.enumerations == 1);
}

void refreshEliminatesRemovedKeys() {
    lo::detail::ChunkSnapshot<int> snapshot;
    ObservedMap                    map{10, 20, 30};
    for (int pick = 0; pick < 400; ++pick) REQUIRE(snapshot.pick(map, static_cast<size_t>(pick) % map.size()));
    map.replace({100, 200, 300, 400});
    for (size_t index = 0; index < map.size(); ++index) {
        auto* key = snapshot.pick(map, index);
        REQUIRE(key);
        REQUIRE(*key == 100 || *key == 200 || *key == 300 || *key == 400);
    }
    REQUIRE(map.enumerations == 2);
}

void snapshotDoesNotKeepChunkPayloadsAlive() {
    lo::detail::ChunkSnapshot<int>         snapshot;
    std::map<int, std::shared_ptr<Policy>> chunks;
    auto                                   chunk    = std::make_shared<Policy>(Policy{1});
    std::weak_ptr<Policy>                  lifetime = chunk;
    chunks.emplace(7, std::move(chunk));
    auto* key = snapshot.pick(chunks, 0);
    REQUIRE(key && *key == 7);
    REQUIRE(!lifetime.expired());
    chunks.clear();
    REQUIRE(lifetime.expired());
    REQUIRE(!snapshot.pick(chunks, 0));

    // A noncopyable mapped value must also be accepted by the real cache helper.
    std::map<int, std::unique_ptr<Policy>> uniqueChunks;
    uniqueChunks.emplace(9, std::make_unique<Policy>(Policy{2}));
    lo::detail::ChunkSnapshot<int> uniqueSnapshot;
    key = uniqueSnapshot.pick(uniqueChunks, 0);
    REQUIRE(key && *key == 9);
}

} // namespace

int main() {
    std::pair<char const*, void (*)()> const tests[]{
        {"no active batch",                     noActiveBatch                              },
        {"deduplicate multiple policies",       deduplicatesPolicies                       },
        {"nested batches flush once",           nestedBatchesFlushAtOuterBoundary          },
        {"restore context before callbacks",    restoresContextBeforeCallbacks             },
        {"discard abandoned outer batch",       exceptionDiscardsAbandonedBatch            },
        {"discard abandoned nested updates",    nestedExceptionDiscardsOnlyAbandonedUpdates},
        {"restore after callback exception",    callbackExceptionRestoresContext           },
        {"thread isolation",                    batchesAreThreadLocal                      },
        {"exact 400 pick refresh",              snapshotRefreshesAfterExactly400Picks      },
        {"empty and invalid picks",             emptyAndInvalidPicksDoNotBuildSnapshots    },
        {"map growth and shrink",               mapGrowthAndShrinkStayWithinSnapshotBounds },
        {"remove stale keys on refresh",        refreshEliminatesRemovedKeys               },
        {"store keys without retaining chunks", snapshotDoesNotKeepChunkPayloadsAlive      },
    };
    int failed{};
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
