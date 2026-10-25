#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <memory>
#include <optional>

namespace lo::features {

namespace detail {
struct LocatorState;
}

// Remember the last sent position for every viewer/observed pair and skip the
// native position/visibility check when nothing changed.
class LocatorOpt final : public Feature<LocatorOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct PairHook;

    LocatorOpt();
    ~LocatorOpt() override;

private:
    std::shared_ptr<detail::LocatorState> mState;
    std::optional<ll::memory::HookRegistrar<PairHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
