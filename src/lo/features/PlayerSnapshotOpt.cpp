// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/dimension.cpp#L33-L146

#include "lo/features/PlayerSnapshotOpt.h"
#include <mc/deps/ecs/WeakEntityRef.h>
#include <mc/deps/ecs/gamerefs_entity/EntityRegistry.h>
#include <mc/deps/ecs/gamerefs_entity/StackResultStorageEntity.h>
#include <mc/entity/components/ActorOwnerComponent.h>
#include <mc/platform/brstd/function_ref.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/dimension/Dimension.h>
#include <limits>
#include <span>
#include <thread>
#include <utility>

namespace lo::features {

std::shared_ptr<PlayerSnapshotOpt::Snapshot> PlayerSnapshotOpt::snapshotFor(Level const& level) {
    if (mRetiring.contains(&level)) return {};

    auto& stored = mSnapshots[&level];
    if (!stored) stored = std::make_shared<Snapshot>();

    auto snapshot = stored;

    auto& users = level.getActiveUsers();
    auto        bytes = std::as_bytes(std::span{users});
    if (snapshot->mState.matches(bytes)) return snapshot;
    if (!snapshot->mState.canRebuild()) return {};

    std::vector<ActiveUser> replacement{};
    replacement.reserve(users.size());
    for (auto const& user : users) {
        auto& entry    = replacement.emplace_back();
        auto  registry = user.mRegistry.lock();
        if (!registry || !registry->mRegistry->valid(user.mEntity)) continue;

        StackResultStorageEntity entity(user);
        if (!entity) continue;

        entry.mEntity.emplace(*entity);
        entry.mRegistry = registry;
        entry.mPlayer   = Player::tryGetFromEntity(*entry.mEntity, true);
    }

    snapshot->mState.replace(bytes);
    snapshot->mEntries.swap(replacement);
    return snapshot;
}

Player* PlayerSnapshotOpt::resolve(ActiveUser& user, bool includeRemoved) {
    auto registry = user.mRegistry.lock();
    if (!registry || !user.mEntity || !registry->mRegistry->valid(user.mEntity->mEntity)) return nullptr;

    // A live registry/entity can temporarily outlive its ActorOwner component;
    // verify the owner before reusing a pointer cached on an earlier call.
    auto owner = user.mEntity->tryGetComponent<ActorOwnerComponent>();
    if (!owner || !owner->mActor) {
        user.mPlayer = nullptr;
        return nullptr;
    }
    if (!user.mPlayer || owner->mActor.get() != user.mPlayer) {
        user.mPlayer = Player::tryGetFromEntity(*user.mEntity, includeRemoved);
    }
    return user.mPlayer && (includeRemoved || !user.mPlayer->mRemoved) ? user.mPlayer : nullptr;
}

void PlayerSnapshotOpt::retire(Level const* level) {
    mRetiring.insert(level);
    if (auto it = mSnapshots.find(level); it != mSnapshots.end()) {
        it->second->mState.retire();
        mSnapshots.erase(it);
    }
}

void PlayerSnapshotOpt::unretire(Level const* level) { mRetiring.erase(level); }

void PlayerSnapshotOpt::clearSnapshots() {
    for (auto& [level, snapshot] : mSnapshots) snapshot->mState.retire();
    mSnapshots.clear();
    mRetiring.clear();
}

LL_TYPE_INSTANCE_HOOK(
    PlayerSnapshotOpt::ForEachPlayerHook,
    HookPriority::Normal,
    Dimension,
    &Dimension::$forEachPlayer,
    void,
    brstd::function_ref<bool(Player&)> callback
) {
    auto self = get();
    if (!self) return origin(callback);

    auto& level    = static_cast<Level&>(mLevel);
    auto  snapshot = self->snapshotFor(level);
    if (!snapshot) return origin(callback);

    auto dimension = getDimensionId();

    detail::SnapshotState::Visit visit(snapshot->mState);
    for (auto& user : snapshot->mEntries) {
        if (snapshot->mState.retired()) break;
        if (auto* player = resolve(user, false);
            player && player->getDimensionId() == dimension && !callback(*player)) {
            break;
        }
    }
}

LL_TYPE_INSTANCE_HOOK(
    PlayerSnapshotOpt::ActivePlayerCountHook,
    HookPriority::Normal,
    Level,
    &Level::$getActivePlayerCount,
    int
) {
    auto self = get();
    if (!self) return origin();

    auto snapshot = self->snapshotFor(*this);
    if (!snapshot || snapshot->mEntries.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return origin();
    }

    detail::SnapshotState::Visit visit(snapshot->mState);
    auto                         count = int{};
    for (auto& user : snapshot->mEntries) {
        if (snapshot->mState.retired()) return origin();
        if (resolve(user, true)) ++count;
    }
    return count;
}

LL_TYPE_INSTANCE_HOOK(
    PlayerSnapshotOpt::LevelDtorHook,
    HookPriority::Normal,
    Level,
    &Level::$dtor,
    void
) {
    auto self = get();
    if (!self) return origin();

    self->retire(this);
    origin();
    self->unretire(this);
}

PlayerSnapshotOpt::PlayerSnapshotOpt()  = default;
PlayerSnapshotOpt::~PlayerSnapshotOpt() = default;

ll::Expected<> PlayerSnapshotOpt::onEnable() {
    if (!mHooks) mHooks.emplace();
    return {};
}

void PlayerSnapshotOpt::onDisable() {
    mHooks.reset();
    clearSnapshots();
}

LO_REGISTER_FEATURE(PlayerSnapshotOpt);

} // namespace lo::features
