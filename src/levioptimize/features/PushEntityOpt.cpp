#include "features.h"
#include "levioptimize/LeviOptimize.h"
#include "ll/api/memory/Hook.h"
#include "mc/entity/components/PushableComponent.h"
#include "mc/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/Level.h"
#include "parallel_hashmap/phmap.h"

namespace lo::push_entity_opt {

static phmap::flat_hash_map<Actor*, int> pushedEntityTimes;

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
    static int maxPushTimes = LeviOptimize::getInstance().getConfig().features.optPushEntity.maxPushTimes;
    if (maxPushTimes == -1) {
        origin(owner, other, pushSelfOnly);
        return;
    }
    auto it = pushedEntityTimes.find(&owner);
    if (it != pushedEntityTimes.end()) {
        if (it->second > maxPushTimes) {
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
    ll::memory::HookRegistrar<PushableComponentPushVec0Opt, PushableComponentPushMaxPushOpt> r;
};

void PushEntityOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
};

PushEntityOpt::PushEntityOpt()  = default;
PushEntityOpt::~PushEntityOpt() = default;

} // namespace lo::push_entity_opt
