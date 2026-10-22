#include "features.h"

#include <vector>

#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/server/ServerLevel.h"
#include "mc/world/level/saveddata/maps/MapItemTrackedActor.h"
#include "mc/world/level/MapDataManager.h"
#include "mc/world/level/saveddata/maps/MapItemSavedData.h"

namespace lo::chunk_leak_fix {

struct ChunkLeakFix::Impl {
    ll::event::ListenerPtr playerLeaveEventListener;

    Impl() {
        auto& eventBus = ll::event::EventBus::getInstance();

        playerLeaveEventListener = eventBus.emplaceListener<ll::event::player::PlayerDisconnectEvent>(
            [](ll::event::player::PlayerDisconnectEvent& event) {
                auto&      player     = event.self();
                auto const playerId   = player.getOrCreateUniqueID();
                auto&      level      = static_cast<ServerLevel&>(player.getLevel());
                auto&      allMapData = level._getMapDataManager().mMapData.get();

                std::vector<MapItemTrackedActor::UniqueId> keys;
                for (auto& [id, data] : allMapData) {
                    if (!data) {
                        continue;
                    }
                    keys.clear();
                    for (auto const& tracker : data->mTrackedEntities.get()) {
                        if (!tracker) {
                            continue;
                        }
                        auto const& key = tracker->mUniqueId.get();
                        if (key.type == MapItemTrackedActor::Type::Entity && key.keyEntityId->rawID == playerId.rawID) {
                            keys.push_back(key);
                        }
                    }
                    // Follow Endstone commit d520c1938: also remove the decoration and mark it dirty.
                    // Copy keys first because the native method invalidates the tracker vector's iterators.
                    for (auto const& key : keys) {
                        data->_removeTrackedMapEntity(key);
                    }
                }
            }
        );
    }

    ~Impl() {
        auto& eventBus = ll::event::EventBus::getInstance();
        eventBus.removeListener(playerLeaveEventListener);
    }
};

void ChunkLeakFix::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
};

ChunkLeakFix::ChunkLeakFix()                                   = default;
ChunkLeakFix::~ChunkLeakFix()                                  = default;
ChunkLeakFix::ChunkLeakFix(ChunkLeakFix&&) noexcept            = default;
ChunkLeakFix& ChunkLeakFix::operator=(ChunkLeakFix&&) noexcept = default;

} // namespace lo::chunk_leak_fix
