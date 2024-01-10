#include "features.h"

#include "ll/api/memory/Hook.h"

#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/utils/StaticVanillaBlocks.h"

namespace lo::random_tick_opt {

LL_TYPE_INSTANCE_HOOK(
    BlockShouldRandomTickHook,
    ll::memory::HookPriority::Normal,
    Block,
    &Block::shouldRandomTick,
    bool
) {
    if (this == StaticVanillaBlocks::mAir || this == StaticVanillaBlocks::mStone || this == StaticVanillaBlocks::mWater
        || this == StaticVanillaBlocks::mNetherrack || this == StaticVanillaBlocks::mDeepslate
        || this == StaticVanillaBlocks::mBedrock) {
        return false;
    }
    return origin();
}

struct RandomTickOpt::Impl {
    ll::memory::HookRegistrar<BlockShouldRandomTickHook> r;
};

void RandomTickOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
};

RandomTickOpt::RandomTickOpt()  = default;
RandomTickOpt::~RandomTickOpt() = default;

} // namespace lo::random_tick_opt