#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <optional>

namespace lo::features {

// Vanilla rescans the whole waypoint group for every player-list entry, for
// every player, every tick. Skip the update when every live player already has
// a waypoint; the native update still handles group creation, toggles and joins.
class VanillaWaypointsOpt final : public Feature<VanillaWaypointsOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct UpdateHook;
    struct WaypointUpdateHook;

    VanillaWaypointsOpt();
    ~VanillaWaypointsOpt() override;

private:
    std::optional<ll::memory::HookRegistrar<UpdateHook, WaypointUpdateHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
