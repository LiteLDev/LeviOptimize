#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <optional>

namespace lo::features {

// BDS keeps relationships to blocks that were already removed when it rebuilds a
// powered block's component list. Before each scene graph update, drop the
// entries that are neither present in the graph nor queued to be added.
class CircuitSceneGraphOpt final : public Feature<CircuitSceneGraphOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct UpdateHook;

    CircuitSceneGraphOpt();
    ~CircuitSceneGraphOpt() override;

private:
    std::optional<ll::memory::HookRegistrar<UpdateHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
