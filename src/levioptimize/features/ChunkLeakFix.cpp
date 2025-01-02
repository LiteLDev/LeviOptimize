#include "features.h"

#include "ll/api/memory/Memory.h"
#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerDisconnectEvent.h>
#include <mc/server/ServerLevel.h>
#include <mc/common/ActorUniqueID.h>
#include <mc/world/level/saveddata/maps/MapItemTrackedActor.h>

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
                    ll::memory::dAccess<std::unordered_map<ActorUniqueID, std::unique_ptr<MapItemSavedData>>>(
                        &manager,
                        112
                    );
                for (auto& [id, data] : allMapData) {
                    auto& v = ll::memory::dAccess<std::vector<std::shared_ptr<MapItemTrackedActor>>>(data.get(), 96);
                    std::erase_if(v, [&player](auto& ptr) {
                        return ll::memory::dAccess<ActorUniqueID>(ptr.get(), 8).rawID
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
