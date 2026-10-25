// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/server_player.cpp#L1-L235

#include "lo/features/ParkedChunksOpt.h"
#include "lo/data/ChunkQueue.h"
#include "lo/data/ParkedQueue.h"
#include "lo/mod/LeviOptimize.h"
#include <ll/api/base/Containers.h>
#include <ll/api/i18n/I18n.h>
#include <mc/server/NetworkChunkPublisher.h>
#include <mc/server/ServerPlayer.h>
#include <mc/world/level/Level.h>
#include <mc/world/level/Tick.h>
#include <mc/world/level/chunk/ChunkViewSource.h>
#include <mc/world/level/chunk/LevelChunk.h>
#include <mc/world/level/chunk/LevelChunkEventManager.h>
#include <cstddef>
#include <memory>

namespace lo::features {

using namespace ll::i18n_literals;

namespace detail {

using namespace ll::i18n_literals;

using data::checkQueueHash;
using data::LoadedJournal;
using data::ParkedQueue;
using data::QueuedChunkState;

struct ParkedPlayerState {
    NetworkChunkPublisher*                                                                mPublisher{};
    ChunkViewSource*                                                                      mSource{};
    ParkedQueue<std::unordered_map<ChunkPositionAndDimension, std::weak_ptr<LevelChunk>>> mQueue;
    size_t                                                                                mLoadedCursor{};
};

// Copyable shim so entt can store the component; the actual state is move-only.
struct ParkedChunksComponent {
    std::shared_ptr<ParkedPlayerState> mPlayer;
};

struct ParkedState {
    enum class HashCheck { Pending, Passed, Failed } mHashCheck{};
    size_t                                          mCheckedKeys{};
    LoadedJournal<ChunkPos>                         mJournal;
    std::unique_ptr<Bedrock::PubSub::Subscription>  mLoadedSubscription;
};

QueuedChunkState resolve(ChunkViewSource& source, ChunkPositionAndDimension const& key) {
    auto chunk = source.getExistingChunk(key.mPos.get());
    if (!chunk) return QueuedChunkState::OutOfView;
    return chunk->mLoadState.get().load() == ChunkState::Loaded ? QueuedChunkState::Sendable
                                                                : QueuedChunkState::NotLoaded;
}

} // namespace detail

LL_TYPE_INSTANCE_HOOK(
    ParkedChunksOpt::UpdateHook,
    HookPriority::Normal,
    ServerPlayer,
    &ServerPlayer::$_updateChunkPublisherView,
    void,
    Vec3 const& position,
    float       minDistance
) {
    auto self = get();
    if (!self) return origin(position, minDistance);

    auto& state     = *self->mState;
    auto* publisher = mChunkPublisherView.get();
    if (!publisher || state.mHashCheck == detail::ParkedState::HashCheck::Failed) {
        return origin(position, minDistance);
    }
    auto previousCenter = ChunkPos(publisher->mLastChunkUpdatePosition.get());
    auto previousRadius = publisher->mLastChunkUpdateRadius;

    origin(position, minDistance);

    auto* source = publisher->mSource.get();
    if (!source) return;

    auto& queue = publisher->mQueuedChunks.get();
    if (state.mHashCheck == detail::ParkedState::HashCheck::Pending) {
        if (queue.size() < 64) return;
        if (!detail::checkQueueHash(queue)) {
            state.mHashCheck = detail::ParkedState::HashCheck::Failed;
            mod::LeviOptimize::getInstance().getSelf().getLogger().warn(
                "parked chunks: native chunk queue hash mismatch; chunks will be sent the slow way."_tr()
            );
            return;
        }
        state.mCheckedKeys += queue.size();
        if (state.mCheckedKeys < 256) return;
        state.mHashCheck = detail::ParkedState::HashCheck::Passed;
        state.mLoadedSubscription.reset(new Bedrock::PubSub::Subscription(
            getLevel().getLevelChunkEventManager()->getOnChunkLoadedConnector().connect(
                [&journal = state.mJournal](ChunkSource&, LevelChunk& chunk, int) {
                    journal.append(chunk.mPosition.get());
                },
                Bedrock::PubSub::ConnectPosition::AtBack,
                nullptr
            )
        ));
    }

    state.mJournal.advance(getLevel().getCurrentServerTick().tickID);

    auto& component = getEntityContext().getOrAddComponent<detail::ParkedChunksComponent>();
    if (!component.mPlayer) component.mPlayer = std::make_shared<detail::ParkedPlayerState>();
    auto& player = *component.mPlayer;
    if (player.mPublisher != publisher) {
        player.mPublisher    = publisher;
        player.mSource       = nullptr;
        player.mLoadedCursor = state.mJournal.end();
        player.mQueue.mParked.clear();
        player.mQueue.mSendable.clear();
    }
    if (player.mSource != source || previousRadius == 0) {
        player.mQueue.restore(queue);
    } else if (
        [] (ChunkPos const& lhs, ChunkPos const& rhs) { return lhs.x != rhs.x || lhs.z != rhs.z; }(
            ChunkPos(publisher->mLastChunkUpdatePosition.get()),
            previousCenter
        )
        || publisher->mLastChunkUpdateRadius != previousRadius
    ) {
        player.mQueue.mSendable.clear();
        player.mQueue.recheckAll(queue, [&](auto const& key) { return detail::resolve(*source, key); });
    }
    player.mSource = source;

    auto dimension = getDimensionId().mValue;
    auto recheck   = [&](auto const& key) { return detail::resolve(*source, key); };
    auto slice     = state.mJournal.since(player.mLoadedCursor);
    if (slice.mMissed) {
        player.mQueue.recheckAll(queue, recheck);
    } else {
        for (auto const& pos : slice.mPositions) {
            ChunkPositionAndDimension key;
            key.mPos  = pos;
            key.mType = DimensionType{dimension};
            auto it   = player.mQueue.mParked.find(key);
            if (it != player.mQueue.mParked.end() && it->second.mState == detail::QueuedChunkState::NotLoaded) {
                player.mQueue.recheck(queue, it, recheck);
            }
        }
    }
    player.mLoadedCursor = slice.mEnd;

    if ((getLevel().getCurrentServerTick().tickID + (reinterpret_cast<uintptr_t>(publisher) >> 4)) % 100 == 0) {
        player.mQueue.recheckAll(queue, recheck);
    }

    player.mQueue.parkUnsendable(queue, [&](auto const& key) { return key.mType->mValue == dimension; }, recheck);
}

ParkedChunksOpt::ParkedChunksOpt() = default;

ParkedChunksOpt::~ParkedChunksOpt() { onDisable(); }

ll::Expected<> ParkedChunksOpt::onEnable() {
    mState = std::make_shared<detail::ParkedState>();
    if (!mHooks) mHooks.emplace();
    return {};
}

void ParkedChunksOpt::onDisable() {
    mHooks.reset();
    mState.reset();
}

LO_REGISTER_FEATURE(ParkedChunksOpt);

} // namespace lo::features
