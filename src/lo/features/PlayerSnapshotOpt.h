#pragma once
#include "lo/features/IFeature.h"
#include "lo/features/detail/PlayerSnapshotState.h"
#include "lo/triggers/ServerTriggers.h"
#include <ll/api/memory/Hook.h>
#include <mc/deps/ecs/gamerefs_entity/EntityContext.h>
#include <memory>
#include <optional>
#include <vector>

class EntityRegistry;
class Level;
class Player;

namespace lo::features {

// Vanilla re-locks every active user's weak reference and looks up three
// components on each forEachPlayer call. Walk a snapshot instead, rebuilt only
// when the level's active-user bytes change.
class PlayerSnapshotOpt final : public Feature<PlayerSnapshotOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    using Trigger     = triggers::ServerStarted;
    using StopTrigger = triggers::ServerStopping;

    struct ForEachPlayerHook;
    struct ActivePlayerCountHook;
    struct LevelDtorHook;

    PlayerSnapshotOpt();
    ~PlayerSnapshotOpt() override;

private:
    struct ActiveUser {
        std::optional<EntityContext>  mEntity;
        std::weak_ptr<EntityRegistry> mRegistry;
        Player*                       mPlayer{};
    };
    struct Snapshot {
        detail::SnapshotState   mState;
        std::vector<ActiveUser> mEntries;
    };

    static Player* resolve(ActiveUser& user, bool includeRemoved);

    std::shared_ptr<Snapshot> snapshotFor(Level const& level);
    void                      retire(Level const* level);
    void                      unretire(Level const* level);
    void                      clearSnapshots();

    ll::SmallDenseMap<Level const*, std::shared_ptr<Snapshot>>                                        mSnapshots;
    ll::SmallDenseSet<Level const*>                                                                   mRetiring;
    std::optional<ll::memory::HookRegistrar<ForEachPlayerHook, ActivePlayerCountHook, LevelDtorHook>> mHooks;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
