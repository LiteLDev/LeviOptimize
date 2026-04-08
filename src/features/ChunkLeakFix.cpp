#include "features.h"

#include "ll/api/memory/Memory.h"
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
            [&](ll::event::player::PlayerDisconnectEvent& event) {
                auto& player  = event.self();
                auto& level   = static_cast<ServerLevel&>(event.self().getLevel());
                auto& manager = level._getMapDataManager();
                auto& allMapData =
                    manager.mMapData.get();
                for (auto& [id, data] : allMapData) {
                    std::erase_if(data->mTrackedEntities.get(), [&player](std::shared_ptr<MapItemTrackedActor>& ptr) {
                        return ptr->mUniqueId->keyEntityId->rawID
                            == player.getOrCreateUniqueID().rawID;
                    });
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

ChunkLeakFix::ChunkLeakFix()  = default;
ChunkLeakFix::~ChunkLeakFix() = default;

} // namespace lo::chunk_leak_fix
