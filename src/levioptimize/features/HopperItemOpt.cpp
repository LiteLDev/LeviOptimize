#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/actor/HopperBlockActor.h"

namespace lo::hopper_item_opt {

LL_TYPE_INSTANCE_HOOK(
    HopperAddItemHook,
    ll::memory::HookPriority::Normal,
    Hopper,
    &Hopper::_addItem,
    bool,
    BlockSource& blockSource,
    Container&   container,
    ItemStack&   item,
    int          face,
    int          itemCount
) {
    auto size = container.getContainerSize();
    for (int slot = 0; slot < size; ++slot) {
        if (!container.canPushInItem(slot, face, item)) {
            continue;
        }
        auto containerItemRef = container.getItemNonConst(slot);
        if (!containerItemRef) {
            continue;
        }
        auto& containerItem = *containerItemRef;
        if (containerItem.mValid_DeprecatedSeeComment && containerItem.mItem && !containerItem.isNull()
            && containerItem.mCount > 0) {
            auto maxSize = containerItem.getMaxStackSize();
            if (containerItem.mCount == maxSize || !containerItem.isStackable(item)) {
                continue;
            }
            if (itemCount + containerItem.mCount <= maxSize) {
                item.remove(itemCount);
                containerItem.add(itemCount);
            } else {
                item.remove(maxSize - containerItem.mCount);
                containerItem.set(maxSize);
            }
        } else {
            containerItem = item;
            containerItem.set(itemCount);
            item.remove(itemCount);
        }
        if (container.getContainerType() == ContainerType::Hopper) {
            ((HopperBlockActor*)((char*)&container - 200))
                ->updateCooldownAfterMove(blockSource.getLevel().getCurrentTick(), mMoveItemSpeed);
        }
        container.setContainerChanged(slot);
        return true;
    }
    return false;
}

struct HopperItemOpt::Impl {
    ll::memory::HookRegistrar<HopperAddItemHook> r;
};

void HopperItemOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

HopperItemOpt::HopperItemOpt()  = default;
HopperItemOpt::~HopperItemOpt() = default;

} // namespace lo::hopper_item_opt
