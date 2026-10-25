#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <optional>

namespace lo::features {

// Vanilla asks every player whether the actor is relevant, doing three unique-id
// lookups and a hash each. Read the actor id once and look it up in each
// player's nearby-actor set instead.
class EntityPacketRecipientsOpt final : public Feature<EntityPacketRecipientsOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct SendPacketHook;

    EntityPacketRecipientsOpt();
    ~EntityPacketRecipientsOpt() override;

private:
    std::optional<ll::memory::HookRegistrar<SendPacketHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
