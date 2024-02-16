#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/entity/components/PushableComponent.h"
#include "mc/math/Vec3.h"
#include "mc/world/actor/Actor.h"

#include <string>

namespace lo::push_entity_opt {

LL_TYPE_INSTANCE_HOOK(
    PushableComponentPushHook,
    ll::memory::HookPriority::Normal,
    PushableComponent,
    "?push@PushableComponent@@QEAAXAEAVActor@@AEBVVec3@@@Z",
    void,
    Actor& owner,
    Vec3&  vec
) {
    if (vec == 0) {
        return;
    }

    origin(owner, vec);
}

struct PushEntityOpt::Impl {
    ll::memory::HookRegistrar<PushableComponentPushHook> r;
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
