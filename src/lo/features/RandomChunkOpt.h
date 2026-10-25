#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include "lo/features/detail/ChunkSnapshot.h"
#include <ll/api/base/Containers.h>
#include <ll/api/memory/Hook.h>
#include <mc/world/level/ChunkPos.h>
#include <optional>

class ChunkSource;

namespace lo::features {

// MainChunkSource::getRandomChunk rebuilds its chunk map on every call; sample
// the keys instead and refresh the sample every few hundred picks, while still
// resolving each sampled key against the live map.
class RandomChunkOpt final : public Feature<RandomChunkOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct RandomChunkHook;

    RandomChunkOpt();
    ~RandomChunkOpt() override;

private:
    ll::SmallDenseMap<ChunkSource const*, detail::ChunkSnapshot<ChunkPos>> mSnapshots;
    std::optional<ll::memory::HookRegistrar<RandomChunkHook>>              mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
