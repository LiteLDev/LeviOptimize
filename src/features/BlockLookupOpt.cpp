#include "parallel_hashmap/phmap.h"

#include "ll/api/memory/Hook.h"

#include "mc/world/level/block/Block.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/server/ServerStartedEvent.h"
#include "mc/world/level/block/registry/BlockTypeRegistry.h"

#include "features.h"

namespace lo::block_lookup_opt {

static phmap::parallel_flat_hash_map<uint64, Block const*>                      blockMap;
static phmap::parallel_flat_hash_map<std::string, SharedPtr<class BlockLegacy>> blockLegacyMap;
static phmap::parallel_flat_hash_map<uint64, SharedPtr<class BlockLegacy>>      blockLegacyHashedMap;

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryGetDefaultBlockStateHook,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    BlockTypeRegistry::getDefaultBlockState,
    Block const&,
    class HashedString const& name,
    bool                      logNotFound
) {
    auto p = blockMap.find(name.hash);
    if (p == blockMap.end()) {
        return origin(name, logNotFound);
    }
    return *(p->second);
}

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryLookupByNameHook,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    &BlockTypeRegistry::lookupByName,
    const Block*,
    class HashedString const&                                                 name,
    std::vector<struct BlockTypeRegistry::BlockComplexAliasBlockState> const& vec,
    bool                                                                      logNotFound
) {
    auto p = blockMap.find(name.hash);
    if (p == blockMap.end()) {
        return origin(name, vec, logNotFound);
    }
    return p->second;
}

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryLookupByNameHook1,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    &BlockTypeRegistry::lookupByName,
    const Block*,
    HashedString const& name,
    int                 data,
    bool                logNotFound
) {
    auto p = blockMap.find(name.hash);
    if (p == blockMap.end()) {
        return origin(name, data, logNotFound);
    }
    return p->second;
}

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryLookupByNameHook2,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    &BlockTypeRegistry::lookupByName,
    WeakPtr<class BlockLegacy>,
    HashedString const& name,
    bool                logNotFound
) {
    auto p = blockLegacyHashedMap.find(name.hash);
    if (p == blockLegacyHashedMap.end()) {
        return origin(name, logNotFound);
    }
    return WeakPtr<class BlockLegacy>(p->second);
}

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryUnregisterBlock,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    &BlockTypeRegistry::unregisterBlock,
    void,
    HashedString const& name
) {
    origin(name);
    blockMap.erase(name.hash);
}

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryUnregisterBlocks,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    &BlockTypeRegistry::unregisterBlocks,
    void
) {
    origin();
    blockMap.clear();
}


struct BlockLookupOpt::Impl {
    ll::memory::HookRegistrar<
        BlockTypeRegistryGetDefaultBlockStateHook,
        BlockTypeRegistryLookupByNameHook,
        BlockTypeRegistryLookupByNameHook1,
        BlockTypeRegistryLookupByNameHook2,
        BlockTypeRegistryUnregisterBlock,
        BlockTypeRegistryUnregisterBlocks>
        r;
};

void initHashMap() {
    for (auto& blkv : BlockTypeRegistry::$mBlockLookupMap()) {
        if (!blockMap.contains(blkv.first.hash)) {
            blockMap[blkv.first.hash] = &blkv.second->getRenderBlock();
        }
        if (!blockLegacyMap.contains(blkv.first.str)) {
            blockLegacyMap[blkv.first.str] = blkv.second;
        }
        if (!blockLegacyHashedMap.contains(blkv.first.hash)) {
            blockLegacyHashedMap[blkv.first.hash] = blkv.second;
        }
    }
}

void clearHashMap() {
    blockMap.clear();
    blockLegacyMap.clear();
    blockLegacyHashedMap.clear();
}

void BlockLookupOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
        initHashMap();
    } else {
        clearHashMap();
    }
};

BlockLookupOpt::BlockLookupOpt()  = default;
BlockLookupOpt::~BlockLookupOpt() = default;

} // namespace lo::block_lookup_opt