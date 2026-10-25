// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/main_chunk_source.cpp#L23-L54

#include "lo/features/RandomChunkOpt.h"
#include <ll/api/memory/Hook.h>
#include <mc/util/Random.h>
#include <mc/world/level/chunk/ChunkState.h>
#include <mc/world/level/chunk/LevelChunk.h>
#include <mc/world/level/chunk/MainChunkSource.h>

namespace lo::features {

LL_TYPE_INSTANCE_HOOK(
    RandomChunkOpt::RandomChunkHook,
    HookPriority::Normal,
    MainChunkSource,
    &MainChunkSource::$getRandomChunk,
    std::shared_ptr<LevelChunk>,
    Random& random
) {
    auto& map = *getChunkMap();
    if (map.empty()) return nullptr;

    auto  index    = static_cast<size_t>(random.nextInt(static_cast<int>(map.size())));
    auto  self     = get();
    auto& snapshot = self->mSnapshots[this];

    auto chunk = getExistingChunk(snapshot.pick(map, index));
    if (!chunk || (*chunk->mLoadState).load() < ChunkState::Loaded) return nullptr;
    return chunk;
}

RandomChunkOpt::RandomChunkOpt()  = default;
RandomChunkOpt::~RandomChunkOpt() = default;

ll::Expected<> RandomChunkOpt::onEnable() {
    if (!mHooks) mHooks.emplace();
    return {};
}

void RandomChunkOpt::onDisable() { mHooks.reset(); }

LO_REGISTER_FEATURE(RandomChunkOpt);

} // namespace lo::features
