// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/core/server.cpp#L240-L251

#include "lo/features/ChunkLeakFix.h"
#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <mc/server/ServerLevel.h>
#include <mc/world/level/MapDataManager.h>
#include <mc/world/level/saveddata/maps/MapItemSavedData.h>
#include <mc/world/level/saveddata/maps/MapItemTrackedActor.h>
#include <ranges>
#include <vector>

namespace lo::features {

ChunkLeakFix::ChunkLeakFix() = default;

ChunkLeakFix::~ChunkLeakFix() { onDisable(); }

ll::Expected<> ChunkLeakFix::onEnable() {
    mListener = ll::event::EventBus::getInstance().emplaceListener<ll::event::PlayerDisconnectEvent>(
        [](ll::event::PlayerDisconnectEvent& event) {
            auto& player     = event.self();
            auto  playerId   = player.getOrCreateUniqueID();
            auto& level      = static_cast<ServerLevel&>(player.getLevel());
            auto& allMapData = *level._getMapDataManager().mMapData;

            std::vector<MapItemTrackedActor::UniqueId> keys{};
            for (auto& data : allMapData | std::views::values) {
                keys.clear();
                for (auto& tracker : *data->mTrackedEntities) {
                    if (!tracker) continue;

                    auto& key = *tracker->mUniqueId;
                    if (key.type == MapItemTrackedActor::Type::Entity && *key.keyEntityId == playerId) {
                        keys.push_back(key);
                    }
                }

                // Copy first: the native method invalidates the tracker vector.
                for (auto& key : keys) data->_removeTrackedMapEntity(key);
            }
        }
    );

    return {};
}

void ChunkLeakFix::onDisable() {
    ll::event::EventBus::getInstance().removeListener<ll::event::PlayerDisconnectEvent>(mListener);
    mListener.reset();
}

LO_REGISTER_FEATURE(ChunkLeakFix);

} // namespace lo::features
