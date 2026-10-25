#pragma once
#include "lo/diagnostics/TimingState.h"
#include "lo/features/IFeature.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <optional>

namespace lo::diagnostics {

// Diagnostic, not an optimization: measure full server-frame MSPT/TPS around
// Minecraft::update and split catch-up Level ticks. No behavior is changed.
class FullFrameTiming final : public features::Feature<FullFrameTiming> {
public:
    using Config = bool;
    static constexpr features::FeatureSide side = features::FeatureSide::server;
    static constexpr std::string_view      section{"diagnostics"};

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct UpdateHook;
    struct LevelTickHook;

    FullFrameTiming();
    ~FullFrameTiming() override;

    [[nodiscard]] detail::TimingSnapshot snapshot() const noexcept { return mTiming.snapshot(); }

private:
    detail::FullFrameTimingState mTiming;
    std::optional<ll::memory::HookRegistrar<UpdateHook, LevelTickHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::diagnostics
