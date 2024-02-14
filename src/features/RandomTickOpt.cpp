#include "features.h"

#include "ll/api/memory/Hook.h"

#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/utils/BedrockBlockNames.h"
#include "mc/world/level/block/utils/VanillaBlockTypeIds.h"

namespace lo::random_tick_opt {

LL_TYPE_INSTANCE_HOOK(
    BlockShouldRandomTickHook,
    ll::memory::HookPriority::Normal,
    Block,
    &Block::shouldRandomTick,
    bool
) {
    auto& name = getName();
    if (name == BedrockBlockNames::Air || name == VanillaBlockTypeIds::Stone || name == VanillaBlockTypeIds::Water
        || name == VanillaBlockTypeIds::Netherrack || name == VanillaBlockTypeIds::Deepslate
        || name == VanillaBlockTypeIds::Bedrock) {
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