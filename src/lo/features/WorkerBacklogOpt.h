#pragma once
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ipatch/ipatch.hpp>
#include <optional>

namespace lo::features {

// Lower the worker pool's minimum backlog from 50 ms to 0 so the pool drains
// small task batches instead of waiting before it starts working.
class WorkerBacklogOpt final : public Feature<WorkerBacklogOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    WorkerBacklogOpt();
    ~WorkerBacklogOpt() override;

private:
    std::optional<ipatch::Patch> mPatch;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
