#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <memory>
#include <optional>

namespace lo::features {

namespace detail {
struct ParkedState;
}

// Keep chunks that cannot be sent yet out of the native publisher queue and put
// them back as soon as they become sendable.
class ParkedChunksOpt final : public Feature<ParkedChunksOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct UpdateHook;

    ParkedChunksOpt();
    ~ParkedChunksOpt() override;

private:
    std::shared_ptr<detail::ParkedState> mState;
    std::optional<ll::memory::HookRegistrar<UpdateHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
