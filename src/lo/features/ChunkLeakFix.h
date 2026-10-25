#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/event/ListenerBase.h>

namespace lo::features {

// Map data keeps tracking entities that already disconnected. On disconnect,
// drop those tracker entries (and the decorations that reference them) so the
// map does not keep the player's entities alive.
class ChunkLeakFix final : public Feature<ChunkLeakFix> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    ChunkLeakFix();
    ~ChunkLeakFix() override;

private:
    ll::event::ListenerPtr mListener;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
