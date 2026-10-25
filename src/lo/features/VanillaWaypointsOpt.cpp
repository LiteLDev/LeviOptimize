// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/vanilla_waypoint_manager.cpp#L26-L53
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/server_waypoint.cpp#L22-L64

#include "lo/features/VanillaWaypointsOpt.h"
#include <mc/legacy/ActorUniqueID.h>
#include <mc/world/ServerWaypoint.h>
#include <mc/world/ServerWaypointGroup.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/actor/player/PlayerListEntry.h>
#include <mc/world/actor/player/VanillaWaypointManager.h>
#include <mc/world/level/Level.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <ranges>
#include <vector>

namespace lo::features {

namespace detail {

bool textureMatches(float distance, int lower, std::optional<int> upper) noexcept {
    return (!upper || static_cast<float>(*upper) > distance) && distance >= static_cast<float>(lower);
}

bool iconChanged(float x, float y, float oldX, float oldY) noexcept {
    constexpr auto epsilon = std::numeric_limits<float>::epsilon();
    return !(epsilon >= std::fabs(x - oldX)) || !(epsilon >= std::fabs(y - oldY));
}

} // namespace detail

LL_TYPE_INSTANCE_HOOK(
    VanillaWaypointsOpt::UpdateHook,
    HookPriority::Normal,
    VanillaWaypointManager,
    &VanillaWaypointManager::update,
    void,
    Player&           self,
    ServerLocatorBar& serverLocatorBar,
    bool              isLocatorBarEnabled
) {
    auto& group = mWaypointGroup->mWaypointGroup;
    if (mIsLocatorBarEnabled == isLocatorBarEnabled && group) {
        std::vector<int64> ids{};
        ids.reserve(group->mWaypoints->size());
        for (auto& waypoint : *group->mWaypoints | std::views::values) {
            if (!waypoint) continue;
            if (auto id = waypoint->tryGetActorID()) ids.push_back(id->rawID);
        }
        std::ranges::sort(ids);

        auto  selfId  = self.getOrCreateUniqueID().rawID;
        auto& level   = self.getLevel();
        auto  missing = std::ranges::any_of(level.getPlayerList(), [&](auto const& entry) {
            auto id = entry.second.mId->rawID;
            return id != selfId && !std::ranges::binary_search(ids, id) && level.fetchEntity(ActorUniqueID{id}, false);
        });
        if (!missing) return;
    }

    origin(self, serverLocatorBar, isLocatorBarEnabled);
}

LL_TYPE_INSTANCE_HOOK(
    VanillaWaypointsOpt::WaypointUpdateHook,
    HookPriority::Normal,
    ServerWaypoint,
    &ServerWaypoint::$update,
    void,
    Player const& viewer
) {
    ServerWaypoint::Texture const* selected{};
    auto&                          textures = mTextureSelector->mTextures.get();
    if (!textures.empty()) {
        auto distance = (mWorldPos->mUnk4d9121.as<Vec3>() - viewer.getPosition()).length();
        for (auto& texture : textures) {
            if (detail::textureMatches(distance, texture.mLowerBound, *texture.mUpperBound)) {
                selected = &texture;
                break;
            }
        }
    }

    if (!selected) {
        if (*mTexturePath) setTexturePath(std::nullopt);
    } else {
        if (*mTexturePath != *selected->mTexturePath) setTexturePath(*selected->mTexturePath);

        auto& size = selected->mIconSize.get();
        if (detail::iconChanged(size.x, size.y, mIconSize->x, mIconSize->y)) {
            mUpdateFlags |= static_cast<uint>(UpdateFlags::IconSize);
            mIconSize->x  = size.x;
            mIconSize->y  = size.y;
        }
    }

    auto visible = calculateIsVisible(viewer);
    if (mIsVisible != visible) {
        mUpdateFlags |= static_cast<uint>(UpdateFlags::Visibility);
        mIsVisible    = visible;
    }
}

VanillaWaypointsOpt::VanillaWaypointsOpt()  = default;
VanillaWaypointsOpt::~VanillaWaypointsOpt() = default;

ll::Expected<> VanillaWaypointsOpt::onEnable() {
    if (!mHooks) mHooks.emplace();
    return {};
}

void VanillaWaypointsOpt::onDisable() { mHooks.reset(); }

LO_REGISTER_FEATURE(VanillaWaypointsOpt);

} // namespace lo::features
