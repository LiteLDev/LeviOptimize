#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/registry/BlockTypeRegistry.h"
#include "parallel_hashmap/phmap.h"

namespace lo::block_lookup_opt {

static phmap::parallel_flat_hash_map<HashedString, Block const*>                 blockMap;
static phmap::parallel_flat_hash_map<HashedString, SharedPtr<class BlockLegacy>> blockLegacyMap;

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryGetDefaultBlockStateHook,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    BlockTypeRegistry::getDefaultBlockState,
    Block const&,
    class HashedString const& name,
    bool                      logNotFound
) {
    auto p = blockMap.find(name);
    if (p == blockMap.end()) {
        return origin(name, logNotFound);
    }
    return *(p->second);
}

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryLookupByNameHook,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    &BlockTypeRegistry::_lookupByNameImpl,
    ::BlockTypeRegistry::LookupByNameImplReturnType,
    class HashedString const&                    name,
    int                                          data,
    ::BlockTypeRegistry::LookupByNameImplResolve resolve,
    bool                                         logNotFound
) {
    if (name.isEmpty()) {
        return {nullptr, nullptr};
    }
    if (resolve == BlockTypeRegistry::LookupByNameImplResolve::BlockLegacy) {
        auto p = blockLegacyMap.find(name);
        if (p == blockLegacyMap.end()) {
            return origin(name, data, resolve, logNotFound);
        }
        return {WeakPtr<BlockLegacy>{p->second}, data, false};
    }
    return origin(name, data, resolve, logNotFound);
}

LL_TYPE_STATIC_HOOK(
    BlockTypeRegistryLookupByNameHook1,
    ll::memory::HookPriority::Normal,
    BlockTypeRegistry,
    &BlockTypeRegistry::lookupByName,
    WeakPtr<class BlockLegacy>,
    HashedString const& name,
    bool                logNotFound
) {
    auto p = blockLegacyMap.find(name);
    if (p == blockLegacyMap.end()) {
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
    blockLegacyMap.erase(name);
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
    blockLegacyMap.clear();
}

struct BlockLookupOpt::Impl {
    ll::memory::HookRegistrar<
        BlockTypeRegistryGetDefaultBlockStateHook,
        BlockTypeRegistryLookupByNameHook,
        BlockTypeRegistryLookupByNameHook1,
        BlockTypeRegistryUnregisterBlock,
        BlockTypeRegistryUnregisterBlocks>
        r;
};

void initHashMap() {
    for (auto& blkv : BlockTypeRegistry::$mBlockLookupMap()) {
        blockLegacyMap.emplace(blkv);
        blockMap.emplace(blkv.first, &blkv.second->getRenderBlock());
    }
}

void clearHashMap() {
    blockMap.clear();
    blockLegacyMap.clear();
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