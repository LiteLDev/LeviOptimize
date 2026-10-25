#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <atomic>
#include <optional>

class LevelChunk;

namespace lo::features {

// Vanilla runs the random-tick loop right after the spawner callback. Warm the
// sub-chunk records, palettes and block event managers it is about to touch,
// using only reads and the live seed, never advancing the game's RNG.
class RandomTickPrefetchOpt final : public Feature<RandomTickPrefetchOpt> {
public:
    using Config                      = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct WarmHook;

    RandomTickPrefetchOpt();
    ~RandomTickPrefetchOpt() override;

private:
    std::atomic<int> const*                            mSeed{};
    std::optional<ll::memory::HookRegistrar<WarmHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
    void           warmRandomTicks(LevelChunk const& chunk);
};

} // namespace lo::features
