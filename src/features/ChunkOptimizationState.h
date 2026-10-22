// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Adapted batching/snapshot state from Endstone commits 099d6f028 and 6f0c32205.
// Modifications: scoped reentrancy/exception handling and bounded snapshot sampling.

#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <ranges>
#include <vector>

namespace lo::detail {

// A batch belongs to one call stack on one thread. Successful nested calls merge
// into their caller; unwinding discards that scope's pointers and restores it.
template <class Policy>
class InfluenceBatch {
    inline static thread_local InfluenceBatch* mActive{};

    InfluenceBatch*      mPrevious = mActive;
    std::vector<Policy*> mPending;
    bool                 mFinished{};

public:
    InfluenceBatch() { mActive = this; }
    ~InfluenceBatch() { mActive = mPrevious; }

    InfluenceBatch(InfluenceBatch const&)            = delete;
    InfluenceBatch& operator=(InfluenceBatch const&) = delete;

    static bool defer(Policy* policy) {
        if (!mActive) return false;
        auto& updates = mActive->mPending;
        if (!std::ranges::contains(updates, policy)) {
            updates.push_back(policy);
        }
        return true;
    }

    template <class Update>
    void finish(Update&& update) {
        if (mFinished) return;
        mFinished = true;
        mActive   = mPrevious;
        if (mPrevious) {
            for (auto* policy : mPending) defer(policy);
        } else {
            for (auto* policy : mPending) update(policy);
        }
    }
};

// Keep keys only: saving must still resolve each key against the live chunk map.
template <class Position>
class ChunkSnapshot {
    static constexpr size_t RefreshInterval = 400;

    std::vector<Position> mPositions;
    size_t                mPicksLeft{};

public:
    template <class Map>
    [[nodiscard]] std::optional<Position> pick(Map const& map, size_t index) {
        if (map.empty() || index >= map.size()) return std::nullopt;
        if (mPicksLeft == 0 || mPositions.empty()) {
            auto replacement = map | std::views::keys | std::ranges::to<std::vector<Position>>();
            mPositions.swap(replacement);
            mPicksLeft = RefreshInterval;
        }
        --mPicksLeft;
        return mPositions[index * mPositions.size() / map.size()];
    }
};

} // namespace lo::detail
