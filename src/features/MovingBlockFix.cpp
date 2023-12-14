#include "features.h"

#include "ll/api/memory/Hook.h"
#include "mc/network/packet/BlockActorDataPacket.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/MovingBlockActor.h"

namespace lo::moving_block_fix {

thread_local bool saveDataFlag = true;

LL_TYPED_INSTANCE_HOOK(
    BlockActorGetServerUpdatePacketHook,
    ll::memory::HookPriority::Normal,
    BlockActor,
    "?getServerUpdatePacket@BlockActor@@QEAA?AV?$unique_ptr@VBlockActorDataPacket@@U?$default_delete@"
    "VBlockActorDataPacket@@@std@@@std@@AEAVBlockSource@@@Z",
    std::unique_ptr<BlockActorDataPacket>,
    BlockSource* bs
) {
    saveDataFlag = false;
    auto rtn     = origin(bs);
    saveDataFlag = true;
    return rtn;
}

LL_TYPED_INSTANCE_HOOK(
    MovingBlockActorSaveHook,
    ll::memory::HookPriority::Normal,
    MovingBlockActor,
    "?save@MovingBlockActor@@UEBA_NAEAVCompoundTag@@@Z",
    bool,
    CompoundTag& tag
) {
    if (saveDataFlag) {
        return origin(tag);
    }

    if (!LL_SYMBOL_CALL("?save@BlockActor@@UEBA_NAEAVCompoundTag@@@Z", bool, BlockActor*, CompoundTag&)(this, tag))
        return false;

    Block* block      = ll::memory::dAccess<Block*>(this, 0xC8);
    Block* extraBlock = ll::memory::dAccess<Block*>(this, 0xD0);

    tag.putCompound("movingBlock", block->getSerializationId().clone());
    tag.putCompound("movingBlockExtra", extraBlock->getSerializationId().clone());
    tag.putInt("pistonPosX", ll::memory::dAccess<int>(this, 58 * sizeof(int)));
    tag.putInt("pistonPosY", ll::memory::dAccess<int>(this, 59 * sizeof(int)));
    tag.putInt("pistonPosZ", ll::memory::dAccess<int>(this, 60 * sizeof(int)));
    tag.putBoolean("expanding", ll::memory::dAccess<bool>(this, 244));

    return true;
}

struct MovingBlockFix::Impl {
    ll::memory::HookRegistrar<BlockActorGetServerUpdatePacketHook, MovingBlockActorSaveHook> r;
};

void MovingBlockFix::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl = nullptr;
    }
}

MovingBlockFix::MovingBlockFix()  = default;
MovingBlockFix::~MovingBlockFix() = default;

} // namespace lo::moving_block_fix
