#include "lo/features/PushEntityOpt.h"
#include <mc/deps/core/math/Vec3.h>
#include <mc/util/PushableByEntityUtility.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/level/Level.h>

namespace lo::features {

LL_STATIC_HOOK(
    PushEntityOpt::Vec0Hook,
    HookPriority::Normal,
    &PushableByEntityUtility::push,
    void,
    Actor&      owner,
    Vec3 const& vec
) {
    if (vec == Vec3{}) return;

    origin(owner, vec);
}

LL_STATIC_HOOK(
    PushEntityOpt::MaxPushHook,
    HookPriority::Normal,
    &PushableByEntityUtility::push,
    void,
    Actor& owner,
    Actor& other,
    bool   pushSelfOnly
) {
    auto self = get();

    auto const& config = self->getConfig();
    if (config.maxPushTimes < 0 || (config.unlimitedPlayerPush && (owner.isPlayer() || other.isPlayer()))) {
        return origin(owner, other, pushSelfOnly);
    }

    auto it = self->mPushedTimes.find(&owner);
    if (it != self->mPushedTimes.end()) {
        if (it->second > config.maxPushTimes) return;
        ++it->second;
    } else {
        self->mPushedTimes[&owner] = 1;
    }

    origin(owner, other, pushSelfOnly);
}

LL_TYPE_INSTANCE_HOOK(PushEntityOpt::TickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();
    get()->mPushedTimes.clear();
}

PushEntityOpt::PushEntityOpt()  = default;
PushEntityOpt::~PushEntityOpt() = default;

ll::Expected<> PushEntityOpt::onEnable() {
    mPushedTimes.clear();
    updateHooks();
    return {};
}

void PushEntityOpt::onDisable() {
    mVec0Hook.reset();
    mMaxPushHook.reset();
    mPushedTimes.clear();
}

void PushEntityOpt::onConfigChanged(Config const&, Config const&) { updateHooks(); }

void PushEntityOpt::updateHooks() {
    auto const& config = getConfig();

    if (config.disableVec0Push) {
        if (!mVec0Hook) mVec0Hook.emplace();
    } else {
        mVec0Hook.reset();
    }

    if (config.maxPushTimes != -1) {
        if (!mMaxPushHook) mMaxPushHook.emplace();
    } else {
        mMaxPushHook.reset();
    }
}

LO_REGISTER_FEATURE(PushEntityOpt);

} // namespace lo::features
