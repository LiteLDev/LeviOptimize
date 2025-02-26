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
                    manager.mUnk8ad806.as<std::unordered_map<ActorUniqueID, std::unique_ptr<MapItemSavedData>>>();
                for (auto& [id, data] : allMapData) {
                    auto& trackedEntities = data->mUnkad59b1.as<std::vector<std::shared_ptr<MapItemTrackedActor>>>();
                    std::erase_if(trackedEntities, [&player](std::shared_ptr<MapItemTrackedActor>& ptr) {
                        return ptr->mUnk6b8326.as<MapItemTrackedActor::UniqueId>().mUnk1cffa7.as<ActorUniqueID>().rawID
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
