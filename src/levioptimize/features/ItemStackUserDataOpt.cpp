#include "features.h"

#include "ll/api/memory/Hook.h"

#include "mc/nbt/CompoundTag.h"
#include "mc/server/ServerPlayer.h"

#include "ll/api/memory/Hook.h"
#include "mc/world/actor/player/Player.h"

#include "mc/world/Container.h"

#include "mc/world/item/components/ComponentItem.h"


#include "mc/world/inventory/transaction/InventoryTransaction.h"

#include "mc/world/inventory/transaction/ItemReleaseInventoryTransaction.h"

#include "mc/world/inventory/transaction/ItemReleaseInventoryTransaction.h"
#include "mc/world/inventory/transaction/ItemUseInventoryTransaction.h"
#include "mc/world/inventory/transaction/ItemUseOnActorInventoryTransaction.h"


#include "mc/world/inventory/transaction/IItemUseTransactionSubject.h"


#include "mc/world/inventory/transaction/InventoryAction.h"
#include "mc/world/inventory/transaction/InventoryTransaction.h"


#include "mc/network/packet/InventoryContentPacket.h"
#include "mc/world/item/NetworkItemInstanceDescriptor.h"

#include "mc/deps/core/utility/ReadOnlyBinaryStream.h"
#include "mc/nbt/NbtIo.h"
#include "mc/util/StringByteInput.h"
#include "mc/util/StringByteOutput.h"

struct ItemStackNetIdVariant;
class NetworkItemStackDescriptor : public ::ItemDescriptorCount {
public:
    ::ll::TypedStorage<1, 1, bool>                     mIncludeNetIds;
    ::ll::TypedStorage<8, 24, ::ItemStackNetIdVariant> mNetIdVariant;
    ::ll::TypedStorage<4, 4, uint>                     mBlockRuntimeId;
    ::ll::TypedStorage<8, 32, ::std::string>           mUserDataBuffer;
};

// #include "mc/world/item/NetworkItemStackDescriptor.h"

namespace ItemStackSerializerHelpers {

template <typename Type>
MCAPI void write(const Type& item, BinaryStream& stream);

template <typename Type>
MCAPI Bedrock::Result<Type, std::error_code> read(ReadOnlyBinaryStream& stream);

MCTAPI void write(const NetworkItemStackDescriptor& item, BinaryStream& stream);
MCTAPI void write(const NetworkItemInstanceDescriptor& item, BinaryStream& stream);

MCTAPI Bedrock::Result<NetworkItemStackDescriptor, std::error_code> read(ReadOnlyBinaryStream& stream);

MCTAPI Bedrock::Result<NetworkItemInstanceDescriptor, std::error_code> read(ReadOnlyBinaryStream& stream);

} // namespace ItemStackSerializerHelpers


namespace lo::itemstack_userdata_opt {


std::string cleanseUserDataBuffer(const std::string& buffer) {
    StringByteInput input(buffer);

    if (input.readShortResult() != -1 || input.readByte() != 1) {
        return {};
    }

    auto tag = NbtIo::read(input);
    if (!tag) return {};

    auto compoundTag = std::move(*tag);

    static const std::unordered_set<std::string> keepKeys{
        "bundle_id",
        "Damage",
        "RepairCost",
        "display",
        "Name",
        "Lore",
        "ench",
        "minecraft:item_lock",
        "Items"
    };

    static const std::unordered_set<std::string> itemKeepKeys{"Count", "Damage", "Name", "Slot"};

    std::map<std::string, CompoundTagVariant, std::less<void>> newTags;

    for (const auto& key : keepKeys) {
        auto it = compoundTag->mTags.find(key);
        if (it == compoundTag->mTags.end()) continue;

        if (key == "Items" && it->second.is_array()) {
            auto listTag = it->second.get<ListTag>();
            for (auto& itemTagPtr : listTag) {
                if (itemTagPtr.is_object()) {
                    auto& itemCompound = itemTagPtr->as<CompoundTag>();
                    std::map<std::string, CompoundTagVariant, std::less<void>> newItemTags;
                    for (const auto& ikey : itemKeepKeys) {
                        auto iit = itemCompound.mTags.find(ikey);
                        if (iit != itemCompound.mTags.end()) {
                            newItemTags.emplace(iit->first, iit->second);
                        }
                    }
                    itemCompound.mTags = std::move(newItemTags);
                }
            }
            newTags.emplace("Items", std::move(listTag));
        } else {
            newTags.emplace(it->first, it->second);
        }
    }

    compoundTag->mTags = std::move(newTags);

    std::string      bufferOutput;
    StringByteOutput output{bufferOutput};
    output.writeShort(-1);
    output.writeByte(1);
    NbtIo::write(compoundTag.get(), output);

    return bufferOutput;
}


LL_STATIC_HOOK(
    ItemStackSerializerHelpersWriteHook,
    HookPriority::Normal,
    &ItemStackSerializerHelpers::write,
    void,
    const NetworkItemStackDescriptor& item,
    BinaryStream&                     stream
) {
    auto& item_ = const_cast<NetworkItemStackDescriptor&>(item);

    item_.mUserDataBuffer = cleanseUserDataBuffer(item.mUserDataBuffer);

    return origin(item_, stream);
}

LL_STATIC_HOOK(
    ItemStackSerializerHelpersWrite2Hook,
    HookPriority::Normal,

    &ItemStackSerializerHelpers::write,
    void,
    const NetworkItemInstanceDescriptor& item,
    BinaryStream&                        stream
) {
    auto& item_ = const_cast<NetworkItemInstanceDescriptor&>(item);

    item_.mUnka0ca51.as<std::string>() = cleanseUserDataBuffer(item_.mUnka0ca51.as<std::string>());

    return origin(item_, stream);
}


struct ItemStackUserDataOpt::Impl {
    ll::memory::HookRegistrar<ItemStackSerializerHelpersWriteHook, ItemStackSerializerHelpersWrite2Hook> r;
};

void ItemStackUserDataOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

ItemStackUserDataOpt::ItemStackUserDataOpt()  = default;
ItemStackUserDataOpt::~ItemStackUserDataOpt() = default;
} // namespace lo::itemstack_userdata_opt