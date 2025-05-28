#include "features.h"

#include "ll/api/memory/Hook.h"

#include "mc/world/item/ItemDescriptorCount.h"

class ItemStack;
class ReadOnlyBinaryStream;
struct ItemStackNetIdVariant;
class NetworkItemStackDescriptor : public ::ItemDescriptorCount {
public:
    ::ll::TypedStorage<1, 1, bool>                     mIncludeNetIds;
    ::ll::TypedStorage<8, 24, ::ItemStackNetIdVariant> mNetIdVariant;
    ::ll::TypedStorage<4, 4, uint>                     mBlockRuntimeId;
    ::ll::TypedStorage<8, 32, ::std::string>           mUserDataBuffer;

public:
    NetworkItemStackDescriptor(NetworkItemStackDescriptor const&);

    virtual ~NetworkItemStackDescriptor() /*override*/;

    MCAPI explicit NetworkItemStackDescriptor(::ItemStack const& item);

    MCAPI ::NetworkItemStackDescriptor& operator=(::NetworkItemStackDescriptor&&);

    MCAPI ::NetworkItemStackDescriptor& operator=(::NetworkItemStackDescriptor const&);
};


#include "mc/network/ServerNetworkHandler.h"
#include "mc/server/ServerPlayer.h"

#include "ll/api/memory/Hook.h"
#include "mc/world/actor/player/Player.h"

#include "mc/world/Container.h"


#include "mc/world/inventory/transaction/InventorySource.h"

#include "mc/world/item/components/ComponentItem.h"

#include "mc/world/containers/DynamicContainerTracker.h"

#include "mc/world/inventory/transaction/InventoryTransaction.h"

#include "mc/world/inventory/transaction/ItemReleaseInventoryTransaction.h"

#include "mc/world/actor/player/Inventory.h"
#include "mc/world/actor/player/PlayerInventory.h"
#include "mc/world/inventory/transaction/ItemReleaseInventoryTransaction.h"
#include "mc/world/inventory/transaction/ItemUseInventoryTransaction.h"
#include "mc/world/inventory/transaction/ItemUseOnActorInventoryTransaction.h"


#include "mc/world/inventory/transaction/InventoryAction.h"
#include "mc/world/inventory/transaction/InventoryTransaction.h"


#include "mc/world/inventory/transaction/IItemUseTransactionSubject.h"


namespace lo::inventory_transaction_patch {

LL_TYPE_INSTANCE_HOOK(
    ItemUseOnActorInventoryTransactionHook,
    HookPriority::Normal,
    ItemUseOnActorInventoryTransaction,
    &ItemUseOnActorInventoryTransaction::$handle,
    ::InventoryTransactionError,
    ::Player& player,
    bool      isSenderAuthority
) {
    auto& inv = player.mInventory;
    if (!isSenderAuthority && inv && inv->mSelectedContainerId == ContainerID::Inventory) {
        if (inv->mSelected < 0 || inv->mSelected >= inv->mInventory->getContainerSize()) {
            return InventoryTransactionError::AuthorityMismatch;
        }
        auto& serverItem = (*(inv->mInventory))[inv->mSelected];
        mItem            = NetworkItemStackDescriptor(serverItem);
    }
    return origin(player, isSenderAuthority);
}

LL_TYPE_INSTANCE_HOOK(
    ItemReleaseInventoryTransactionHook,
    HookPriority::Normal,
    ItemReleaseInventoryTransaction,
    &ItemReleaseInventoryTransaction::$handle,
    ::InventoryTransactionError,
    ::Player& player,
    bool      isSenderAuthority
) {
    auto& inv = player.mInventory;
    if (!isSenderAuthority && inv && inv->mSelectedContainerId == ContainerID::Inventory) {
        if (inv->mSelected < 0 || inv->mSelected >= inv->mInventory->getContainerSize()) {
            return InventoryTransactionError::AuthorityMismatch;
        }
        auto& serverItem = (*(inv->mInventory))[inv->mSelected];
        mItem            = NetworkItemStackDescriptor(serverItem);
        // player.refreshInventory();
    }

    return origin(player, isSenderAuthority);
}


struct PlayerInventorySlotData {
    ContainerID mContainerId;
    int         mSlot;
};

LL_TYPE_INSTANCE_HOOK(
    ItemUseInventoryTransactionHook,
    HookPriority::Normal,
    ItemUseInventoryTransaction,
    &ItemUseInventoryTransaction::$handle,
    ::InventoryTransactionError,
    ::Player& player,
    bool      isSenderAuthority
) {

    auto& inv = player.mInventory;
    if (!isSenderAuthority && inv && inv->mSelectedContainerId == ContainerID::Inventory) {
        if (mSlot < 0 || mSlot >= inv->mInventory->getContainerSize()) {
            return InventoryTransactionError::AuthorityMismatch;
        }
        auto& serverItem = (*(inv->mInventory))[mSlot];
        mItem            = NetworkItemStackDescriptor(serverItem);
        // player.refreshInventory();
    }

    return origin(player, isSenderAuthority);
}


LL_TYPE_INSTANCE_HOOK(
    InventoryTransactionVerifyHook,
    HookPriority::Normal,
    InventoryTransaction,
    &InventoryTransaction::verifyFull,
    ::InventoryTransactionError,
    ::Player& player,
    bool      isSenderAuthority
) {
    if (isSenderAuthority) {
        return origin(player, isSenderAuthority);
    }

    if (mActions->size() != 2) {
        return InventoryTransactionError::AuthorityMismatch;
    }

    InventoryAction* dropAction  = nullptr;
    InventoryAction* carryAction = nullptr;
    uchar            dropCount   = 0;
    bool             foundDrop   = false;


    for (const auto& [source, actions] : *mActions) {
        if (actions.size() != 1) {
            return InventoryTransactionError::AuthorityMismatch;
        }

        const auto& action = actions[0];
        auto        type   = source.mType;
        auto        cid    = source.mContainerId;

        if (type == InventorySourceType::WorldInteraction && cid == ContainerID::None) {
            dropAction = const_cast<InventoryAction*>(&action);
            dropCount  = action.mToItem->mCount;
            foundDrop  = true;
        } else if (type == InventorySourceType::ContainerInventory && cid == ContainerID::Inventory) {
            carryAction = const_cast<InventoryAction*>(&action);
        } else {
            return InventoryTransactionError::AuthorityMismatch;
        }
    }

    if (!dropAction || !carryAction || !foundDrop) {
        return InventoryTransactionError::AuthorityMismatch;
    }

    int countDiff = carryAction->mFromItem->mCount - carryAction->mToItem->mCount;
    if (countDiff != dropCount) {
        return InventoryTransactionError::AuthorityMismatch;
    }

    auto serverItem = player.getCarriedItem();
    if (serverItem.mCount < dropCount) {
        return InventoryTransactionError::AuthorityMismatch;
    }

    carryAction->mFromItem           = serverItem;
    carryAction->mFromItemDescriptor = NetworkItemStackDescriptor(serverItem);

    auto droppedItem   = serverItem;
    droppedItem.mCount = dropCount;

    dropAction->mToItem           = droppedItem;
    dropAction->mToItemDescriptor = NetworkItemStackDescriptor(droppedItem);

    return origin(player, isSenderAuthority);
}

LL_TYPE_INSTANCE_HOOK(
    InventoryTransactionExecuteFullHook,
    HookPriority::Normal,
    InventoryTransaction,
    &InventoryTransaction::executeFull,
    ::InventoryTransactionError,
    ::Player& player,
    bool      isSenderAuthority
) {

    if (isSenderAuthority) {
        return origin(player, isSenderAuthority);
    }

    auto originResult = origin(player, isSenderAuthority);

    // player.refreshInventory();

    return originResult;
}

struct InventoryTransactionPatch::Impl {
    ll::memory::HookRegistrar<
        ItemUseOnActorInventoryTransactionHook,
        ItemReleaseInventoryTransactionHook,
        ItemUseInventoryTransactionHook,
        InventoryTransactionVerifyHook,
        InventoryTransactionExecuteFullHook>
        r;
};

void InventoryTransactionPatch::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

InventoryTransactionPatch::InventoryTransactionPatch()  = default;
InventoryTransactionPatch::~InventoryTransactionPatch() = default;
} // namespace lo::inventory_transaction_patch