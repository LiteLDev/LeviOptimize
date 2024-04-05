#include "features.h"

#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include "mc/certificates/Certificate.h"
#include "mc/certificates/ExtendedCertificate.h"
#include "mc/network/ServerNetworkHandler.h"
#include "mc/server/ServerInstance.h"
#include "mc/world/ActorUniqueID.h"
#include "mc/world/level/Level.h"


#include "parallel_hashmap/phmap.h"
namespace lo::player_lookup_opt {

static phmap::parallel_flat_hash_map<mce::UUID, Player*>   playersCacheByUUID;
static phmap::parallel_flat_hash_map<int64, Player*>       playersCacheByUniqueID;
static phmap::parallel_flat_hash_map<std::string, Player*> playersCacheByName;

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
        playersCacheByUUID.emplace(uuid, pl);
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
        playersCacheByUniqueID.emplace(entityID.id, pl);
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
        playersCacheByName.emplace(name, pl);
        return pl;
    } else {
        return nullptr;
    }
}


LL_TYPE_INSTANCE_HOOK(
    PlayerHook,
    ll::memory::HookPriority::Normal,
    Player,
    "??0Player@@QEAA@AEAVLevel@@AEAVPacketSender@@W4GameType@@_NAEBVNetworkIdentifier@@W4SubClientId@@VUUID@mce@@AEBV?$"
    "basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@7V?$unique_ptr@VCertificate@@U?$default_delete@"
    "VCertificate@@@std@@@9@AEAVEntityContext@@77@Z",
    Player*,
    class Level&                       level,
    class PacketSender&                sender,
    ::GameType                         gameType,
    bool                               bool1,
    class NetworkIdentifier const&     networkIdentifier,
    ::SubClientId                      subClientId,
    class mce::UUID const&             uuid,
    std::string const&                 str1,
    std::string const&                 str2,
    std::unique_ptr<class Certificate> certificate,
    class EntityContext&               entityContext,
    std::string const&                 platformOfflineId,
    std::string const&                 platformDeviceId

) {
    auto res = origin(
        level,
        sender,
        gameType,
        bool1,
        networkIdentifier,
        subClientId,
        uuid,
        str1,
        str2,
        std::move(certificate),
        entityContext,
        platformOfflineId,
        platformDeviceId
    );
    playersCacheByName.emplace(getName(), res);
    playersCacheByUUID.emplace(uuid, res);
    return res;
}

LL_TYPE_INSTANCE_HOOK(
    ServerNetworkHandlerCreateNewPlayerHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::_createNewPlayer,
    ServerPlayer&,
    class NetworkIdentifier const&          networkIdentifier,
    class SubClientConnectionRequest const& request,
    ::SubClientId                           subClientId
) {
    auto& res = origin(networkIdentifier, request, subClientId);
    playersCacheByUniqueID.emplace(res.getOrCreateUniqueID().id, &res);
    return res;
}

LL_TYPE_INSTANCE_HOOK(
    ServerNetworkHandlerTrytLoadPlayerHook,
    ll::memory::HookPriority::Normal,
    ServerNetworkHandler,
    &ServerNetworkHandler::trytLoadPlayer,
    bool,
    class ServerPlayer&            player,
    class ConnectionRequest const& connectionRequest
) {
    auto res = origin(player, connectionRequest);
    playersCacheByUniqueID.emplace(player.getOrCreateUniqueID().id, &player);
    return res;
}


LL_TYPE_INSTANCE_HOOK(PlayerLeaveEventHook, HookPriority::Normal, ServerPlayer, &ServerPlayer::disconnect, void) {
    playersCacheByUUID.erase(getUuid());
    playersCacheByUniqueID.erase(getOrCreateUniqueID().id);
    playersCacheByName.erase(getRealName());
    origin();
}

struct PlayerLookupOpt::Impl {
    ll::memory::HookRegistrar<
        LevelQueryPlayer,
        LevelQueryPlayer1,
        LevelQueryPlayer2,
        PlayerHook,
        ServerNetworkHandlerCreateNewPlayerHook,
        ServerNetworkHandlerTrytLoadPlayerHook,
        PlayerLeaveEventHook>
        r;
};

void PlayerLookupOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
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