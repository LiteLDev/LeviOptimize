#include "features.h"
#include "ll/api/memory/Hook.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/LeavesBlock.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/redstone/circuit/CircuitSceneGraph.h"
#include "mc/world/redstone/circuit/components/BaseCircuitComponent.h"
#include "mc/world/redstone/circuit/components/CircuitComponentList.h"

namespace lo::power_association_map_leak_fix {

LL_TYPE_INSTANCE_HOOK(
    RemoveStaleRelationshipsHook,
    ll::memory::HookPriority::Normal,
    CircuitSceneGraph,
    &CircuitSceneGraph::removeStaleRelationships,
    void
) {
    for (auto iterUpdate = mPendingUpdates.begin(); iterUpdate != mPendingUpdates.end(); iterUpdate++) {
        BlockPos& posUpdate            = *iterUpdate->second.mPos;
        auto      powerAssociationIter = mPowerAssociationMap.find(posUpdate);
        if (powerAssociationIter != mPowerAssociationMap.end()) {
            CircuitComponentList& relationships = powerAssociationIter->second;
            for (auto perChunkIter = relationships.mComponents.begin();
                 perChunkIter != relationships.mComponents.end();) {
                perChunkIter     = relationships.mComponents.erase(perChunkIter);
                auto allListIter = mAllComponents.find(perChunkIter->mPos);
                if (allListIter != mAllComponents.end()) {
                    allListIter->second->removeSource(posUpdate, iterUpdate->second.mRawComponentPtr);
                }
            }
        }
    }
}

struct PowerAssociationMapLeakFix::Impl {
    ll::memory::HookRegistrar<RemoveStaleRelationshipsHook> r;
};

void PowerAssociationMapLeakFix::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

PowerAssociationMapLeakFix::PowerAssociationMapLeakFix()  = default;
PowerAssociationMapLeakFix::~PowerAssociationMapLeakFix() = default;


} // namespace lo::power_association_map_leak_fix