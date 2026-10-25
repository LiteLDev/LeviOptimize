#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <optional>

namespace lo::features {

// Every player's networking pass calls ChunkBuildOrderPolicy::updateInfluences()
// on its own; defer those calls to the end of the pass and run each policy once.
class PlayerNetworkingOpt final : public Feature<PlayerNetworkingOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct PlayerNetworkingHook;
    struct UpdateInfluencesHook;

    PlayerNetworkingOpt();
    ~PlayerNetworkingOpt() override;

private:
    std::optional<ll::memory::HookRegistrar<PlayerNetworkingHook, UpdateInfluencesHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
