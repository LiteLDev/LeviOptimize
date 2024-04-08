#include "features.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"

#include "mc/certificates/ExtendedCertificate.h"
#include "mc/deps/core/mce/UUID.h"
#include "mc/entity/gamerefs_entity/EntityRegistry.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/server/ServerInstance.h"
#include "mc/server/ServerLevel.h"
#include "mc/world/ActorUniqueID.h"
#include "mc/world/level/Level.h"

#include "parallel_hashmap/phmap.h"

namespace lo::player_lookup_opt {

static phmap::flat_hash_map<mce::UUID, Player*>   playersCacheByUUID;
static phmap::flat_hash_map<int64, Player*>       playersCacheByUniqueID;
static phmap::flat_hash_map<std::string, Player*> playersCacheByName;

LL_TYPE_INSTANCE_HOOK(
    LevelQueryPlayer,
    ll::memory::HookPriority::Normal,
    Level,
    &Level::getPlayer,
    Player*,
    class mce::UUID const& uuid
) {
    auto it = playersCacheByUUID.find(uuid);
    if (it != playersCacheByUUID.end()) {
        return it->second;
    } else if (auto pl = origin(uuid); pl) {
        return pl;
    } else {
        return nullptr;
    }
}

LL_TYPE_INSTANCE_HOOK(
    LevelQueryPlayer1,
    ll::memory::HookPriority::Normal,
    Level,
    &Level::getPlayer,
    Player*,
    struct ActorUniqueID entityID
) {
    auto it = playersCacheByUniqueID.find(entityID.id);
    if (it != playersCacheByUniqueID.end()) {
        return it->second;
    } else if (auto pl = origin(entityID); pl) {
        return pl;
    } else {
        return nullptr;
    }
}

LL_TYPE_INSTANCE_HOOK(
    LevelQueryPlayer2,
    ll::memory::HookPriority::Normal,
    Level,
    &Level::getPlayer,
    Player*,
    std::string const& name
) {
    auto it = playersCacheByName.find(name);
    if (it != playersCacheByName.end()) {
        return it->second;
    } else if (auto pl = origin(name); pl) {
        return pl;
    } else {
        return nullptr;
    }
}

#include "mc/world/actor/Actor.h"

LL_TYPE_INSTANCE_HOOK(
    ServerLevel_onGameplayUserAdded,
    ll::memory::HookPriority::Normal,
    ServerLevel,
    &ServerLevel::_onGameplayUserAdded,
    void,
    class EntityContext& entity
) {
    auto ac = Actor::tryGetFromEntity(entity, true);
    if (ac) {
        playersCacheByUUID.emplace(((Player*)ac)->getUuid(), (Player*)ac);
        playersCacheByUniqueID.emplace(ac->getOrCreateUniqueID().id, (Player*)ac);
        playersCacheByName.emplace(((Player*)ac)->getName(), (Player*)ac);
    }
    return origin(entity);
}


LL_TYPE_INSTANCE_HOOK(
    ServerLevel_onGameplayUserRemoved,
    ll::memory::HookPriority::Normal,
    ServerLevel,
    &ServerLevel::_onGameplayUserRemoved,
    void,
    class EntityContext& entity
) {
    auto ac = Actor::tryGetFromEntity(entity, true);
    if (ac) {
        playersCacheByUUID.erase(((Player*)ac)->getUuid());
        playersCacheByUniqueID.erase(ac->getOrCreateUniqueID().id);
        playersCacheByName.erase(((Player*)ac)->getName());
    }
    return origin(entity);
}

struct PlayerLookupOpt::Impl {
    ll::memory::HookRegistrar<
        LevelQueryPlayer,
        LevelQueryPlayer1,
        LevelQueryPlayer2,
        ServerLevel_onGameplayUserAdded,
        ServerLevel_onGameplayUserRemoved>
        r;
};

void PlayerLookupOpt::call(bool enable) {
    if (enable) {
        if (!impl) {
            impl       = std::make_unique<Impl>();
            auto level = ll::service::getLevel();
            if (level) {
                level->forEachUser([&](EntityContext& pl) {
                    auto ac = Actor::tryGetFromEntity(pl, true);
                    if (ac) {
                        playersCacheByUUID.emplace(((Player*)ac)->getUuid(), (Player*)ac);
                        playersCacheByUniqueID.emplace(ac->getOrCreateUniqueID().id, (Player*)ac);
                        playersCacheByName.emplace(((Player*)ac)->getName(), (Player*)ac);
                    }
                    return true;
                });
            }
        }
    } else {
        impl.reset();
        playersCacheByUUID.clear();
        playersCacheByUniqueID.clear();
        playersCacheByName.clear();
    }
}

PlayerLookupOpt::PlayerLookupOpt()  = default;
PlayerLookupOpt::~PlayerLookupOpt() = default;

} // namespace lo::player_lookup_opt