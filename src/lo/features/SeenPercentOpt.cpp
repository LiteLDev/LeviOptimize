#include "lo/features/SeenPercentOpt.h"
#include <mc/world/level/BlockSource.h>
#include <mc/world/level/Level.h>

namespace lo::features {

LL_TYPE_INSTANCE_HOOK(
    SeenPercentOpt::SeenPercentHook,
    HookPriority::Normal,
    BlockSource,
    &BlockSource::getSeenPercent,
    float,
    Vec3 const& center,
    AABB const& bb
) {
    SeenPercentKey key{getDimensionId(), center, bb};

    auto self = get();

    if (auto it = self->mSeenPercentCache.find(key); it != self->mSeenPercentCache.end()) {
        return it->second;
    }

    auto result = origin(center, bb);
    self->mSeenPercentCache.emplace(std::move(key), result);
    return result;
}

LL_TYPE_INSTANCE_HOOK(SeenPercentOpt::LevelTickHook, HookPriority::Normal, Level, &Level::$tick, void) {
    origin();
    get()->mSeenPercentCache.clear();
}


SeenPercentOpt::SeenPercentOpt()  = default;
SeenPercentOpt::~SeenPercentOpt() = default;

ll::Expected<> SeenPercentOpt::onEnable() {
    if (!mHooks) mHooks.emplace();
    return {};
}

void SeenPercentOpt::onDisable() { mHooks.reset(); }

LO_REGISTER_FEATURE(SeenPercentOpt);

} // namespace lo::features
