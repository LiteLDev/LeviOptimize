#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/_HeaderOutputPredefine.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/phys/AABB.h"
#include "parallel_hashmap/phmap.h"
#include <tuple>


namespace lo::seen_percent_opt {

static phmap::parallel_flat_hash_map<std::tuple<DimensionType, Vec3, AABB>, float> explosionVisibilityCache;

LL_AUTO_TYPE_INSTANCE_HOOK(
    BlockSourcegetSeenPercentHook,
    ll::memory::HookPriority::Normal,
    BlockSource,
    &BlockSource::getSeenPercent,
    float,
    class Vec3 const& center,
    class AABB const& bb
) {
    std::tuple<DimensionType, Vec3, AABB> key{this->getDimensionId(), center, bb};

    auto it = explosionVisibilityCache.find(key);
    if (it != explosionVisibilityCache.end()) {
        return it->second;
    }
    float result                  = origin(center, bb);
    explosionVisibilityCache[key] = result;
    return result;
}


LL_AUTO_TYPE_INSTANCE_HOOK(TickHook, ll::memory::HookPriority::Normal, Level, &Level::tick, void) {
    origin();
    explosionVisibilityCache.clear();
}

struct SeenPercentOpt::Impl {
    ll::memory::HookRegistrar<BlockSourcegetSeenPercentHook, TickHook> r;
};

void SeenPercentOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

SeenPercentOpt::SeenPercentOpt()  = default;
SeenPercentOpt::~SeenPercentOpt() = default;

} // namespace lo::seen_percent_opt