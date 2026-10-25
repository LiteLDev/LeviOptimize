#pragma once
#include <ll/api/base/Containers.h>
#include <ll/api/base/StdInt.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

namespace lo::data {

enum class QueuedChunkState { Sendable, OutOfView, NotLoaded };

template <class Queue>
bool checkQueueHash(Queue const& queue) {
    size_t count{};
    for (auto it = queue.begin(); it != queue.end(); ++it, ++count) {
        auto found = queue.find(it->first);
        if (found == queue.end() || &*found != &*it) return false;
    }
    return count == queue.size();
}

template <class Queue>
class ParkedQueue {
public:
    using Key = typename Queue::key_type;
    struct ParkedChunk {
        typename Queue::node_type mNode;
        QueuedChunkState          mState;
    };
    using Parked = ll::SmallDenseMap<Key, ParkedChunk, typename Queue::hasher, typename Queue::key_equal>;
    Parked                                                                     mParked;
    ll::SmallDenseSet<Key, typename Queue::hasher, typename Queue::key_equal> mSendable;

    typename Parked::iterator unpark(Queue& queue, typename Parked::iterator it) {
        if (auto live = queue.find(it->first); live != queue.end()) {
            if (!it->second.mNode.mapped().expired()) live->second = it->second.mNode.mapped();
        } else {
            queue.insert(std::move(it->second.mNode));
        }
        return mParked.erase(it);
    }

    void restore(Queue& queue) {
        for (auto it = mParked.begin(); it != mParked.end();) it = unpark(queue, it);
        mSendable.clear();
    }

    template <class Resolve>
    typename Parked::iterator recheck(Queue& queue, typename Parked::iterator it, Resolve&& resolve) {
        if (it->second.mNode.mapped().expired()) return mParked.erase(it);
        if (queue.contains(it->first)) return unpark(queue, it);
        auto state = resolve(it->first);
        if (state == QueuedChunkState::Sendable) return unpark(queue, it);
        it->second.mState = state;
        return std::next(it);
    }

    template <class Resolve>
    void recheckAll(Queue& queue, Resolve&& resolve) {
        for (auto it = mParked.begin(); it != mParked.end();) it = recheck(queue, it, resolve);
    }

    template <class Belongs, class Resolve>
    void parkUnsendable(Queue& queue, Belongs&& belongs, Resolve&& resolve) {
        phmap::erase_if(mSendable, [&](auto const& key) { return !queue.contains(key); });
        for (auto it = queue.begin(); it != queue.end();) {
            if (mSendable.contains(it->first) || it->second.expired() || !belongs(it->first)) {
                ++it;
                continue;
            }
            auto state = resolve(it->first);
            if (state == QueuedChunkState::Sendable) {
                mSendable.insert(it->first);
                ++it;
                continue;
            }
            auto key               = it->first;
            auto node              = queue.extract(it++);
            auto [entry, inserted] = mParked.try_emplace(key);
            if (inserted || entry->second.mNode.mapped().expired()) {
                entry->second = ParkedChunk{std::move(node), state};
            }
        }
    }
};

// The journal retains the same two-tick event window as Endstone, with absolute
// cursors. Late clients request a complete recheck instead of losing load events.
template <class Position, size_t MaxEntries = 65536>
class LoadedJournal {
    static_assert(MaxEntries > 0);
    std::vector<Position> mLoaded;
    size_t                mBegin{};
    size_t                mSeen{};
    uint64                mTick{};
    bool                  mInitialized{};

public:
    struct Slice {
        bool                  mMissed;
        size_t                mEnd;
        std::vector<Position> mPositions;
    };
    void append(Position const& position) {
        // Streaming can continue without players calling the consumer hook.
        // Bound event memory and let older cursors request a complete recheck.
        if (mLoaded.size() == MaxEntries) {
            auto remove = mLoaded.size() / 2 + 1;
            mLoaded.erase(mLoaded.begin(), mLoaded.begin() + static_cast<std::ptrdiff_t>(remove));
            mBegin += remove;
            mSeen   = std::max(mSeen, mBegin);
        }
        mLoaded.push_back(position);
    }
    void advance(uint64 currentTick) {
        if (mInitialized && currentTick == mTick) return;
        mTick        = currentTick;
        mInitialized = true;
        auto remove  = std::min(mSeen - mBegin, mLoaded.size());
        mLoaded.erase(mLoaded.begin(), mLoaded.begin() + static_cast<std::ptrdiff_t>(remove));
        mBegin += remove;
        mSeen   = mBegin + mLoaded.size();
    }
    [[nodiscard]] size_t end() const { return mBegin + mLoaded.size(); }
    [[nodiscard]] Slice  since(size_t cursor) const {
        if (cursor < mBegin || cursor > end()) return {true, end(), {}};
        return {
            false,
            end(),
            {mLoaded.begin() + static_cast<std::ptrdiff_t>(cursor - mBegin), mLoaded.end()}
        };
    }
};

} // namespace lo::data
