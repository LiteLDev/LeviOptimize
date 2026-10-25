// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/core/server.cpp#L870-L903

#include "lo/diagnostics/FullFrameTiming.h"
#include <ll/api/service/Bedrock.h>
#include <mc/world/Minecraft.h>
#include <mc/world/level/Level.h>

namespace lo::diagnostics {

LL_TYPE_INSTANCE_HOOK(
    FullFrameTiming::UpdateHook,
    HookPriority::Highest,
    Minecraft,
    &Minecraft::update,
    bool
) {
    auto self = get();
    if (self) self->mTiming.beginFrame(detail::FullFrameTimingState::Clock::now());
    auto result = origin();
    if (self) self->mTiming.endFrame(detail::FullFrameTimingState::Clock::now());
    return result;
}

LL_TYPE_INSTANCE_HOOK(
    FullFrameTiming::LevelTickHook,
    HookPriority::Highest,
    Level,
    &Level::$tick,
    void
) {
    auto self = get();
    if (!self) return origin();

    auto currentLevel = ll::service::getLevel();
    if (!currentLevel || &*currentLevel != this) return origin();

    self->mTiming.beginTick(detail::FullFrameTimingState::Clock::now());
    origin();
    self->mTiming.completeTick();
}

FullFrameTiming::FullFrameTiming() = default;

FullFrameTiming::~FullFrameTiming() = default;

ll::Expected<> FullFrameTiming::onEnable() {
    if (!mHooks) mHooks.emplace();
    return {};
}

void FullFrameTiming::onDisable() {
    mHooks.reset();
    mTiming.reset();
}

LO_REGISTER_FEATURE(FullFrameTiming);

} // namespace lo::diagnostics
