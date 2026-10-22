// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Adapted from Endstone commit fa7b3d4eb488c5180bf722d7777b12536e35cc50.
// Modifications: retain native forEachPlayer, use only per-send state, validate
// the SDK's ActorUniqueID hash, and install the fast path on dedicated servers.

#include "features.h"
#include "LeviOptimize.h"

#ifdef LL_PLAT_S
#include "EntityPacketRecipientsState.h"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

#include "ll/api/memory/Hook.h"
#include "mc/entity/components/ActorUniqueIDComponent.h"
#include "mc/entity/components/UserEntityIdentifierComponent.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/network/NetworkIdentifierWithSubId.h"
#include "mc/network/PacketSender.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/ILevel.h"
#include "mc/world/level/dimension/Dimension.h"
#endif

namespace lo::entity_packet_recipients_opt {

#ifdef LL_PLAT_S
namespace {
bool hasCompatibleActorIdHash() {
    // Endstone c54592af5 established the native 1.26.51 hash: hash_combine of
    // the signed high half and the unsigned low 32 bits. Early 26.51 SDK headers
    // accidentally masked to 24 bits, which misses native unordered_map entries.
    constexpr std::array<std::int64_t, 7> probes{0, 1, -1, 0x1000000, 0x1FFFFFFFF, 0x123456789ABCDEF, -0x123456789};
    auto                                  hashPart = std::hash<std::int64_t>{};
    for (auto rawId : probes) {
        size_t expected{};
        expected ^= hashPart(rawId >> 32) + 0x9e3779b9 + (expected << 6) + (expected >> 2);
        expected ^= hashPart(rawId & 0xFFFFFFFF) + 0x9e3779b9 + (expected << 6) + (expected >> 2);
        if (std::hash<ActorUniqueID>{}(ActorUniqueID{rawId}) != expected) return false;
    }
    return true;
}
} // namespace

LL_TYPE_INSTANCE_HOOK(
    EntityPacketRecipientsHook,
    ll::memory::HookPriority::Normal,
    Dimension,
    &Dimension::$sendPacketForEntity,
    void,
    Actor const&  actor,
    Packet const& packet,
    Player const* except
) {
    auto const invalidId = ActorUniqueID::INVALID_ID();
    auto       actorId   = actor.getEntityContext().tryGetComponent<ActorUniqueIDComponent>();
    detail::EntityPacketRecipients<NetworkIdentifierWithSubId> recipients{
        actorId && actorId->mActorUniqueID.get() != invalidId
    };
    if (!recipients.canCollect()) {
        origin(actor, packet, except);
        return;
    }
    auto const id     = actorId->mActorUniqueID.get();
    auto*      sender = mLevel.getPacketSender();
    if (!sender) {
        origin(actor, packet, except);
        return;
    }

    // Native iteration retains its active-player and dimension filtering. This
    // option substitutes vanilla relevance checks; mods overriding those checks
    // should keep it disabled.
    forEachPlayer([&](Player& player) {
        if (&player == except) return true;
        auto playerId = player.getEntityContext().tryGetComponent<ActorUniqueIDComponent>();
        auto valid    = playerId && playerId->mActorUniqueID.get() != invalidId;
        auto same     = valid && playerId->mActorUniqueID.get() == id;
        auto nearby   = valid && !same && static_cast<ServerPlayer&>(player).mNearbyActors->contains(id);
        return recipients.visit(false, valid, same, nearby, [&]() -> std::optional<NetworkIdentifierWithSubId> {
            auto user = player.getEntityContext().tryGetComponent<UserEntityIdentifierComponent>();
            if (!user) return std::nullopt;
            return NetworkIdentifierWithSubId{user->mNetworkId.get(), user->mClientSubId};
        });
    });
    recipients.finish(
        [&](auto const& ids) { sender->sendToClients(ids, packet); },
        [&] { origin(actor, packet, except); }
    );
}
#endif

struct EntityPacketRecipientsOpt::Impl {
#ifdef LL_PLAT_S
    std::optional<ll::memory::HookRegistrar<EntityPacketRecipientsHook>> hook;

    Impl() {
        if (!hasCompatibleActorIdHash()) {
            getLogger().warn(
                "optEntityPacketRecipients: incompatible ActorUniqueID hash; keeping native packet routing."
            );
            return;
        }
        hook.emplace();
    }
#else
    Impl() {
        getLogger().warn("optEntityPacketRecipients is server-only; keeping native packet routing on the client.");
    }
#endif
};

void EntityPacketRecipientsOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

EntityPacketRecipientsOpt::EntityPacketRecipientsOpt()                                                = default;
EntityPacketRecipientsOpt::~EntityPacketRecipientsOpt()                                               = default;
EntityPacketRecipientsOpt::EntityPacketRecipientsOpt(EntityPacketRecipientsOpt&&) noexcept            = default;
EntityPacketRecipientsOpt& EntityPacketRecipientsOpt::operator=(EntityPacketRecipientsOpt&&) noexcept = default;

} // namespace lo::entity_packet_recipients_opt
