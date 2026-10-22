// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Adapted for LeviLamina from EndstoneMC/endstone commit 55e5e34be.

#include "features.h"

#include <algorithm>

#include "ll/api/memory/Hook.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/redstone/circuit/CircuitSceneGraph.h"
#include "mc/world/redstone/circuit/components/CircuitComponentList.h"

namespace lo::circuit_scene_graph_opt {

static_assert(sizeof(CircuitSceneGraph) == 408);
static_assert(sizeof(CircuitComponentList::Item) == 32);

LL_TYPE_INSTANCE_HOOK(
    CircuitSceneGraphUpdateHook,
    ll::memory::HookPriority::Normal,
    CircuitSceneGraph,
    &CircuitSceneGraph::update,
    void,
    BlockSource* region
) {
    // BDS keeps relationships to removed blocks when rebuilding a powered block's list.
    // Preserve components that are still present or waiting to be added this update.
    for (auto const& [pos, entry] : mPendingUpdates) {
        auto it = mPowerAssociationMap.find(entry.mPos.get());
        if (it == mPowerAssociationMap.end()) {
            continue;
        }
        std::erase_if(it->second.mComponents, [this](CircuitComponentList::Item const& item) {
            return !mAllComponents.contains(item.mPos) && !mPendingAdds.contains(item.mPos);
        });
    }
    origin(region);
}

struct CircuitSceneGraphOpt::Impl {
    ll::memory::HookRegistrar<CircuitSceneGraphUpdateHook> hook;
};

void CircuitSceneGraphOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

CircuitSceneGraphOpt::CircuitSceneGraphOpt()                                           = default;
CircuitSceneGraphOpt::~CircuitSceneGraphOpt()                                          = default;
CircuitSceneGraphOpt::CircuitSceneGraphOpt(CircuitSceneGraphOpt&&) noexcept            = default;
CircuitSceneGraphOpt& CircuitSceneGraphOpt::operator=(CircuitSceneGraphOpt&&) noexcept = default;

} // namespace lo::circuit_scene_graph_opt
