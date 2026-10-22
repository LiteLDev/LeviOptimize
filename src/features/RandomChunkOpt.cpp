// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Adapted from Endstone commits 6f0c32205 and bec9a2fd7.
// Modifications: remove cached keys on source destruction/disable, synchronize the
// cache, bound the random index, and use LeviLamina's named chunk state field.

#include "features.h"
#include "ChunkOptimizationState.h"

#include <limits>
#include <mutex>
#include <unordered_map>

#include "ll/api/memory/Hook.h"
#include "mc/util/Random.h"
#include "mc/world/level/chunk/ChunkState.h"
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/world/level/chunk/MainChunkSource.h"

namespace lo::random_chunk_opt {

namespace {
std::mutex                                                              snapshotMutex;
std::unordered_map<ChunkSource const*, detail::ChunkSnapshot<ChunkPos>> snapshots;
} // namespace

LL_TYPE_INSTANCE_HOOK(
    RandomChunkHook,
    ll::memory::HookPriority::Normal,
    MainChunkSource,
    &MainChunkSource::$getRandomChunk,
    std::shared_ptr<LevelChunk>,
    Random& random
) {
    auto const& map = mChunkMap.get();
    if (map.size() > static_cast<size_t>(std::numeric_limits<int>::max())) return origin(random);

    if (map.empty()) {
        std::lock_guard lock(snapshotMutex);
        snapshots.erase(this);
        return nullptr;
    }
    // Preserve BDS's single bounded draw, including when a cached key expired.
    // Invoke native code without holding the snapshot cache lock.
    auto     index = static_cast<size_t>(random.nextInt(static_cast<int>(map.size())));
    ChunkPos position;
    {
        std::lock_guard lock(snapshotMutex);
        auto*           selected = snapshots[this].pick(map, index);
        if (!selected) return nullptr;
        position = *selected;
    }
    auto chunk = getExistingChunk(position);
    if (!chunk || chunk->mLoadState.get().load() < ChunkState::Loaded) return nullptr;
    return chunk;
}

LL_TYPE_INSTANCE_HOOK(ChunkSourceDtorHook, ll::memory::HookPriority::Normal, ChunkSource, &ChunkSource::$dtor, void) {
    {
        std::lock_guard lock(snapshotMutex);
        snapshots.erase(this);
    }
    origin();
}

struct RandomChunkOpt::Impl {
    ll::memory::HookRegistrar<RandomChunkHook, ChunkSourceDtorHook> hooks;

    ~Impl() {
        std::lock_guard lock(snapshotMutex);
        snapshots.clear();
    }
};

void RandomChunkOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

RandomChunkOpt::RandomChunkOpt()                                     = default;
RandomChunkOpt::~RandomChunkOpt()                                    = default;
RandomChunkOpt::RandomChunkOpt(RandomChunkOpt&&) noexcept            = default;
RandomChunkOpt& RandomChunkOpt::operator=(RandomChunkOpt&&) noexcept = default;

} // namespace lo::random_chunk_opt
