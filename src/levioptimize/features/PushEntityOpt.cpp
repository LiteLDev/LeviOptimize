#include "features.h"
#include "levioptimize/LeviOptimize.h"
#include "ll/api/memory/Hook.h"
#include "mc/entity/components/PushableComponent.h"
#include "mc/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/Level.h"
#include "parallel_hashmap/phmap.h"

namespace lo::push_entity_opt {

LL_TYPE_INSTANCE_HOOK(
    PushableComponentPushVec0Opt,
    ll::memory::HookPriority::Normal,
    PushableComponent,
    &PushableComponent::push,
    void,
    class Actor&      owner,
    class Vec3 const& vec
) {
    if (vec == 0) {
        return;
    }

    origin(owner, vec);
}

static phmap::flat_hash_map<Actor*, int> pushedEntityTimes;

LL_TYPE_INSTANCE_HOOK(
    PushableComponentPushMaxPushOpt,
    ll::memory::HookPriority::Normal,
    PushableComponent,
    &PushableComponent::push,
    void,
    class Actor& owner,
    class Actor& other,
    bool         pushSelfOnly
) {
    auto& config = LeviOptimize::getInstance().getConfig().features.optPushEntity.storage;
    if (config.maxPushTimes < 0 || (config.unlimitedPlayerPush && (owner.isPlayer() || other.isPlayer()))) {
        origin(owner, other, pushSelfOnly);
        return;
    }
    auto it = pushedEntityTimes.find(&owner);
    if (it != pushedEntityTimes.end()) {
        if (it->second > config.maxPushTimes) {
            return;
        }
        it->second++;
    } else {
        pushedEntityTimes[&owner] = 1;
    }
    origin(owner, other, pushSelfOnly);
}


LL_TYPE_INSTANCE_HOOK(TickHook, ll::memory::HookPriority::Normal, Level, &Level::tick, void) {
    origin();
    pushedEntityTimes.clear();
}

struct PushEntityOpt::Impl {
    std::optional<ll::memory::HookRegistrar<PushableComponentPushVec0Opt>>              vec0hook;
    std::optional<ll::memory::HookRegistrar<PushableComponentPushMaxPushOpt, TickHook>> pushtimehook;
};

void PushEntityOpt::call(Config const& config) {
    if (config.enable) {
        if (!impl) impl = std::make_unique<Impl>();
        if (config.disableVec0Push) {
            if (!impl->vec0hook) {
                impl->vec0hook.emplace();
            }
        } else {
            impl->vec0hook.reset();
        }
        if (config.maxPushTimes != -1) {
            if (!impl->pushtimehook) {
                impl->pushtimehook.emplace();
            }
        } else {
            impl->pushtimehook.reset();
        }
    } else {
        impl.reset();
    }
};

PushEntityOpt::PushEntityOpt()  = default;
PushEntityOpt::~PushEntityOpt() = default;

} // namespace lo::push_entity_opt
