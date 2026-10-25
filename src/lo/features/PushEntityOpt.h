#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/base/Containers.h>
#include <ll/api/memory/Hook.h>
#include <optional>

class Actor;

namespace lo::features {

// Two independent push fixes: drop pushes with a zero vector, and cap how often
// one entity can push in a tick. Each half is installed only while its config
// field asks for it.
class PushEntityOpt final : public Feature<PushEntityOpt> {
public:
    struct Config {
        bool enable              = true;
        bool disableVec0Push     = true;
        int  maxPushTimes        = -1;
        bool unlimitedPlayerPush = false;
    };
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct Vec0Hook;
    struct MaxPushHook;
    struct TickHook;

    PushEntityOpt();
    ~PushEntityOpt() override;

    void onConfigChanged(Config const&, Config const&);

private:
    ll::SmallDenseMap<Actor*, int> mPushedTimes;

    std::optional<ll::memory::HookRegistrar<Vec0Hook>>             mVec0Hook;
    std::optional<ll::memory::HookRegistrar<MaxPushHook, TickHook>> mMaxPushHook;

    void updateHooks();

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
