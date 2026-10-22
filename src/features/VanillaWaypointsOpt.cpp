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
// Adapted from Endstone commit 874c1fa310545487cc86f7f7f5c0e207514bdb3f.
// Modifications: use LeviLamina's named fields and public hooks, with local actor IDs only.

#include "features.h"
#include "VanillaWaypointsState.h"

#include <cstdint>
#include <ranges>
#include <utility>
#include <vector>

#include "ll/api/memory/Hook.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/world/ServerWaypoint.h"
#include "mc/world/ServerWaypointGroup.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/player/PlayerListEntry.h"
#include "mc/world/actor/player/VanillaWaypointManager.h"
#include "mc/world/level/Level.h"

namespace lo::vanilla_waypoints_opt {

static_assert(sizeof(VanillaWaypointManager) == 24);
static_assert(sizeof(ServerLocatorBar::GroupStrongHandle) == 16);

LL_TYPE_INSTANCE_HOOK(
    VanillaWaypointManagerUpdateHook,
    ll::memory::HookPriority::Normal,
    VanillaWaypointManager,
    &VanillaWaypointManager::update,
    void,
    Player&           self,
    ServerLocatorBar& serverLocatorBar,
    bool              isLocatorBarEnabled
) {
    auto const& group = mWaypointGroup->mWaypointGroup;
    if (mIsLocatorBarEnabled == isLocatorBarEnabled && group) {
        std::vector<std::int64_t> ids;
        ids.reserve(group->mWaypoints->size());
        for (auto const& waypoint : group->mWaypoints.get() | std::views::values) {
            if (!waypoint) continue;
            if (auto const id = waypoint->tryGetActorID()) {
                ids.push_back(id->rawID);
            }
        }
        auto const  selfId    = self.getOrCreateUniqueID().rawID;
        auto const& level     = self.getLevel();
        auto        playerIds = level.getPlayerList() | std::views::values
                              | std::views::transform([](PlayerListEntry const& entry) { return entry.mId->rawID; });
        if (!detail::hasMissingLiveWaypoint(std::move(ids), playerIds, selfId, [&](std::int64_t id) {
                return level.fetchEntity(ActorUniqueID{id}, false) != nullptr;
            })) {
            return;
        }
    }
    // Native update still handles initial group creation, toggle changes, and newly joined live players.
    origin(self, serverLocatorBar, isLocatorBarEnabled);
}

struct VanillaWaypointsOpt::Impl {
    ll::memory::HookRegistrar<VanillaWaypointManagerUpdateHook> hook;
};

void VanillaWaypointsOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

VanillaWaypointsOpt::VanillaWaypointsOpt()                                          = default;
VanillaWaypointsOpt::~VanillaWaypointsOpt()                                         = default;
VanillaWaypointsOpt::VanillaWaypointsOpt(VanillaWaypointsOpt&&) noexcept            = default;
VanillaWaypointsOpt& VanillaWaypointsOpt::operator=(VanillaWaypointsOpt&&) noexcept = default;

} // namespace lo::vanilla_waypoints_opt
