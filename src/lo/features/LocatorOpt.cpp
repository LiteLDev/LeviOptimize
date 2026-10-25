// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/player_location_sender.cpp

#include "lo/features/LocatorOpt.h"
#include "lo/features/detail/LocatorState.h"
#include <ll/api/base/Containers.h>
#include <mc/deps/core/utility/NonOwnerPointer.h>
#include <mc/deps/ecs/WeakEntityRef.h>
#include <mc/deps/ecs/gamerefs_entity/EntityContext.h>
#include <mc/entity/components/UserEntityIdentifierComponent.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/actor/player/PlayerListEntry.h>
#include <mc/world/level/GameplayUserManager.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/PlayerListManager.h>
#include <mc/world/level/PlayerLocationSender.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace lo::features {

namespace detail {

using Action  = std::function<void(UserEntityIdentifierComponent const&, Player const&, Player const&)>;
using Pair    = std::pair<ActorUniqueID, ActorUniqueID>;
using SentMap = std::remove_reference_t<decltype(std::declval<PlayerLocationSender&>().mSentPlayerData.get())>;

struct LocatorState {
    PlayerLocationSender*                          mOwner{};
    SentPositions<Vec3>                            mPositions;
    std::unique_ptr<Bedrock::PubSub::Subscription> mRemovalSubscription;
};

class SentWindow {
    SentMap&                                                  mMap;
    VectorWindow<SentMap::key_container_type, SentMap::mapped_container_type> mWindow;

    static SentMap::key_container_type seedKeys(Pair const& pair, SentPosition<Vec3> const* entry) {
        return entry && entry->mState == SentState::Position ? SentMap::key_container_type{pair}
                                                            : SentMap::key_container_type{};
    }
    static SentMap::mapped_container_type seedValues(SentPosition<Vec3> const* entry) {
        return entry && entry->mState == SentState::Position ? SentMap::mapped_container_type{entry->mPosition}
                                                            : SentMap::mapped_container_type{};
    }

public:
    SentWindow(SentMap& map, Pair const& pair, SentPosition<Vec3> const* entry)
    : mMap(map),
      mWindow(
          const_cast<SentMap::key_container_type&>(map.keys()),
          const_cast<SentMap::mapped_container_type&>(map.values()),
          seedKeys(pair, entry),
          seedValues(entry)
      ) {}

    [[nodiscard]] auto const& keys() const { return mMap.keys(); }
    [[nodiscard]] auto const& values() const { return mMap.values(); }
};

} // namespace detail

LL_TYPE_INSTANCE_HOOK(
    LocatorOpt::PairHook,
    HookPriority::Normal,
    PlayerLocationSender,
    &PlayerLocationSender::_forEachClientPlayerPair,
    void,
    std::vector<WeakEntityRef> const& users,
    detail::Action                    action
) {
    auto self = get();
    if (!self) return origin(users, std::move(action));
    auto& state = *self->mState;

    auto kind = detail::classify(action.target_type().raw_name());
    if (kind == detail::PairAction::Other) return origin(users, std::move(action));

    struct Observed {
        ActorUniqueID                                   mId{};
        size_t                                          mSlot{};
        PlayerLocationSender::PlayerLocationData const* mData{};
    };

    PlayerLocationSender* sender{};
    Player const*         lastViewer{};
    ActorUniqueID         viewerId{};
    size_t                viewerSlot{};
    DimensionType         viewerDimension{};
    ll::SmallDenseMap<Player const*, Observed> observedPlayers;

    auto bindSender = [&](Player const& viewer) {
        if (sender) return;
        auto manager = viewer.getLevel().getPlayerListManager();
        sender       = manager->mPlayerLocationSender->get();
        if (!sender) return;
        if (sender != state.mOwner) {
            state.mPositions.clear();
            state.mOwner = sender;
        }
        if (state.mRemovalSubscription) return;

        auto* level = &viewer.getLevel();
        state.mRemovalSubscription.reset(new Bedrock::PubSub::Subscription(
            level->getGameplayUserManager()->getGameplayUserRemovedConnector().connect(
                [&state, level](EntityContext& entity) {
                    if (!state.mOwner) return;
                    auto user = entity.tryGetComponent<UserEntityIdentifierComponent>();
                    if (!user) return;
                    auto* current = level->getPlayerListManager()->mPlayerLocationSender->get();
                    if (!current || current != state.mOwner) return;
                    auto uuid = user->mClientUUID.get();
                    for (auto const& [key, entry] : level->getPlayerList()) {
                        if (key == uuid) {
                            state.mPositions.remove(entry.mId.get().rawID);
                            break;
                        }
                    }
                },
                Bedrock::PubSub::ConnectPosition::AtFront,
                nullptr
            )
        ));
    };

    auto shouldSend = [&](
                         Player const&                                  viewer,
                         std::optional<Vec3> const&                     previous,
                         PlayerLocationSender::PlayerLocationData const& data
                     ) {
        return sender->_shouldSendPositionPacket(viewer.getPosition(), viewerDimension, false, previous, data);
    };

    auto runAction = [&](
                         UserEntityIdentifierComponent const&        user,
                         Player const&                              viewer,
                         Player const&                              observed,
                         detail::SentPosition<Vec3> const*          entry
                     ) -> detail::SentPosition<Vec3> {
        if (!sender) {
            action(user, viewer, observed);
            return {};
        }
        auto pair = detail::Pair{viewerId, observed.getOrCreateUniqueID()};

        detail::SentWindow window(sender->mSentPlayerData.get(), pair, entry);
        action(user, viewer, observed);

        detail::SentPosition<Vec3> updated{};
        auto const&                keys   = window.keys();
        auto const&                values = window.values();
        for (size_t i = 0; i < keys.size(); ++i) {
            if (keys[i] != pair) continue;
            if (auto const& position = values[i]) updated = {*position, detail::SentState::Position};
            else updated.mState = detail::SentState::Nullopt;
        }
        return updated;
    };

    auto hidePair = [&](UserEntityIdentifierComponent const& user, Player const& viewer, Player const& observed) {
        bindSender(viewer);
        runAction(user, viewer, observed, nullptr);
    };

    auto checkPair = [&](UserEntityIdentifierComponent const& user, Player const& viewer, Player const& observed) {
        bindSender(viewer);
        if (!sender) {
            action(user, viewer, observed);
            return;
        }
        if (&viewer != lastViewer) {
            lastViewer      = &viewer;
            viewerId        = viewer.getOrCreateUniqueID();
            viewerSlot      = state.mPositions.slot(viewerId.rawID);
            viewerDimension = viewer.getDimensionId();
        }

        auto [it, inserted] = observedPlayers.try_emplace(&observed);
        auto& observedPlayer = it->second;
        if (inserted) {
            observedPlayer.mId   = observed.getOrCreateUniqueID();
            observedPlayer.mSlot = state.mPositions.slot(observedPlayer.mId.rawID);

            auto const& current = sender->mCurrentPlayerLocationData.get();
            auto const& keys    = current.keys();
            auto        key     = std::ranges::lower_bound(keys, observedPlayer.mId.rawID, {}, &ActorUniqueID::rawID);
            observedPlayer.mData = key != keys.end() && *key == observedPlayer.mId
                                    ? &current.values()[key - keys.begin()]
                                    : nullptr;
        }

        auto& entry = state.mPositions.at(viewerSlot, observedPlayer.mSlot);
        auto* data  = observedPlayer.mData;
        if (data && entry.mState != detail::SentState::Absent && !data->mIsSpectator) {
            if (entry.mState == detail::SentState::Nullopt) {
                if (!shouldSend(viewer, std::nullopt, *data)) return;
            } else if (
                data->mPosition.get()
                && static_cast<int>(data->mDimension.get()) == static_cast<int>(viewerDimension)
                && !shouldSend(viewer, entry.mPosition, *data)
            )
                return;
        }

        entry = runAction(user, viewer, observed, &entry);
    };

    if (kind == detail::PairAction::Hide) {
        origin(users, hidePair);
        state.mPositions.clear();
        state.mOwner = nullptr;
        return;
    }
    origin(users, checkPair);
}

LocatorOpt::LocatorOpt() = default;

LocatorOpt::~LocatorOpt() { onDisable(); }

ll::Expected<> LocatorOpt::onEnable() {
    mState = std::make_shared<detail::LocatorState>();
    if (!mHooks) mHooks.emplace();
    return {};
}

void LocatorOpt::onDisable() {
    mHooks.reset();
    mState.reset();
}

LO_REGISTER_FEATURE(LocatorOpt);

} // namespace lo::features
