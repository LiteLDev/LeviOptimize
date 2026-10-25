// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/dimension.cpp#L83-L117

#include "lo/features/EntityPacketRecipientsOpt.h"
#include <mc/entity/components/ActorUniqueIDComponent.h>
#include <mc/entity/components/UserEntityIdentifierComponent.h>
#include <mc/legacy/ActorUniqueID.h>
#include <mc/network/NetworkIdentifierWithSubId.h>
#include <mc/network/PacketSender.h>
#include <mc/server/ServerPlayer.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/ILevel.h>
#include <mc/world/level/dimension/Dimension.h>
#include <vector>

namespace lo::features {

LL_TYPE_INSTANCE_HOOK(
    EntityPacketRecipientsOpt::SendPacketHook,
    HookPriority::Normal,
    Dimension,
    &Dimension::$sendPacketForEntity,
    void,
    Actor const&  actor,
    Packet const& packet,
    Player const* except
) {
    static auto invalidId = ActorUniqueID::INVALID_ID();

    auto actorId = actor.getEntityContext().tryGetComponent<ActorUniqueIDComponent>();
    if (!actorId || *actorId->mActorUniqueID == invalidId) return origin(actor, packet, except);

    auto id = *actorId->mActorUniqueID;

    auto* sender = mLevel.getPacketSender();
    if (!sender) return origin(actor, packet, except);

    std::vector<NetworkIdentifierWithSubId> recipients{};
    auto                                    fallback = false;

    forEachPlayer([&](Player& player) {
        if (&player == except) return true;

        auto playerId = player.getEntityContext().tryGetComponent<ActorUniqueIDComponent>();
        if (!playerId || *playerId->mActorUniqueID == invalidId) {
            // A player without a usable id cannot be classified; let the native
            // path handle this send instead of dropping or duplicating a client.
            fallback = true;
            return false;
        }

        auto same   = *playerId->mActorUniqueID == id;
        auto nearby = !same && static_cast<ServerPlayer&>(player).mNearbyActors->contains(id);
        if (!same && !nearby) return true;

        if (auto user = player.getEntityContext().tryGetComponent<UserEntityIdentifierComponent>()) {
            recipients.push_back({*user->mNetworkId, user->mClientSubId});
        }
        return true;
    });

    if (fallback) {
        return origin(actor, packet, except);
    }
    sender->sendToClients(recipients, packet);
}

EntityPacketRecipientsOpt::EntityPacketRecipientsOpt()  = default;
EntityPacketRecipientsOpt::~EntityPacketRecipientsOpt() = default;

ll::Expected<> EntityPacketRecipientsOpt::onEnable() {
    if (!mHooks) mHooks.emplace();
    return {};
}

void EntityPacketRecipientsOpt::onDisable() { mHooks.reset(); }

LO_REGISTER_FEATURE(EntityPacketRecipientsOpt);

} // namespace lo::features
