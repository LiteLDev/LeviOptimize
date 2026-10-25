// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/circuit_scene_graph.cpp#L22-L38

#include "lo/features/CircuitSceneGraphOpt.h"
#include <mc/world/level/BlockSource.h>
#include <mc/world/redstone/circuit/CircuitSceneGraph.h>
#include <mc/world/redstone/circuit/components/CircuitComponentList.h>

namespace lo::features {

LL_TYPE_INSTANCE_HOOK(
    CircuitSceneGraphOpt::UpdateHook,
    HookPriority::Normal,
    CircuitSceneGraph,
    &CircuitSceneGraph::update,
    void,
    BlockSource* region
) {
    for (auto& pending : mPendingUpdates | std::views::values) {
        auto it = mPowerAssociationMap.find(*pending.mPos);
        if (it == mPowerAssociationMap.end()) continue;

        std::erase_if(it->second.mComponents, [this](CircuitComponentList::Item const& item) {
            return !mAllComponents.contains(item.mPos) && !mPendingAdds.contains(item.mPos);
        });
    }
    origin(region);
}

CircuitSceneGraphOpt::CircuitSceneGraphOpt()  = default;
CircuitSceneGraphOpt::~CircuitSceneGraphOpt() = default;

ll::Expected<> CircuitSceneGraphOpt::onEnable() {
    if (!mHooks) mHooks.emplace();
    return {};
}

void CircuitSceneGraphOpt::onDisable() { mHooks.reset(); }

LO_REGISTER_FEATURE(CircuitSceneGraphOpt);

} // namespace lo::features
