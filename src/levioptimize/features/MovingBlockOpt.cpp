#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/network/packet/BlockActorDataPacket.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/MovingBlockActor.h"
#include "mc/world/level/BlockPos.h"

namespace lo::moving_block_opt {

thread_local bool updatePacketFlag = false;

LL_TYPE_INSTANCE_HOOK(
    BlockActorGetServerUpdatePacketHook,
    ll::memory::HookPriority::Normal,
    BlockActor,
    &BlockActor::getServerUpdatePacket,
    std::unique_ptr<BlockActorDataPacket>,
    BlockSource& bs
) {
    updatePacketFlag = true;
    auto rtn         = origin(bs);
    updatePacketFlag = false;
    return rtn;
}

LL_TYPE_INSTANCE_HOOK(
    MovingBlockActorSaveHook,
    ll::memory::HookPriority::Normal,
    MovingBlockActor,
    &MovingBlockActor::$save,
    bool,
    ::CompoundTag&       tag,
    const ::SaveContext& saveContext
) {
    if (!updatePacketFlag) {
        return origin(tag, saveContext);
    }

    if (!::BlockActor::save(tag, saveContext)) return false; // NOLINT

    tag["movingBlock"]      = mWrappedBlock->getSerializationId();
    tag["movingBlockExtra"] = mWrappedExtraBlock->getSerializationId();

    tag["pistonPosX"] = mPosition->x;
    tag["pistonPosY"] = mPosition->y;
    tag["pistonPosZ"] = mPosition->z;
    tag["expanding"]  = mPistonBlockExpanding;

    return true;
}

struct MovingBlockOpt::Impl {
    ll::memory::HookRegistrar<BlockActorGetServerUpdatePacketHook, MovingBlockActorSaveHook> r;
};

void MovingBlockOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

MovingBlockOpt::MovingBlockOpt()  = default;
MovingBlockOpt::~MovingBlockOpt() = default;

} // namespace lo::moving_block_opt
