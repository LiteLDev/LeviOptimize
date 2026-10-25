#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <mc/world/level/dimension/DimensionType.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/base/Containers.h>
#include <mc/world/phys/AABB.h>
#include <optional>

namespace lo::features {

class SeenPercentOpt final : public Feature<SeenPercentOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct SeenPercentHook;
    struct LevelTickHook;
    using SeenPercentKey = std::tuple<DimensionType, Vec3, AABB>;

    SeenPercentOpt();
    ~SeenPercentOpt() override;

private:
    ll::SmallDenseMap<SeenPercentKey, float>                                 mSeenPercentCache;
    std::optional<ll::memory::HookRegistrar<SeenPercentHook, LevelTickHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
