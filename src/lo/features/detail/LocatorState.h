#pragma once
#include <ll/api/base/Containers.h>
#include <ll/api/base/StdInt.h>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace lo::features::detail {

enum class PairAction : uint8 { Other, Check, Hide };

inline PairAction classify(std::string_view name) noexcept {
    if (name.find("checkPlayerPairsAndMaybeSendPackets") != std::string_view::npos) return PairAction::Check;
    if (name.find("sendPacketsHidingAllPlayers") != std::string_view::npos) return PairAction::Hide;
    return PairAction::Other;
}

// Swap both vectors for the duration of one native action, then restore them.
template <class Keys, class Values>
class VectorWindow {
    Keys&       mKeys;
    Values&     mValues;
    Keys        mRealKeys, mWindowKeys;
    Values      mRealValues, mWindowValues;

public:
    VectorWindow(Keys& keys, Values& values, Keys seedKeys = {}, Values seedValues = {})
    : mKeys(keys),
      mValues(values),
      mWindowKeys(std::move(seedKeys)),
      mWindowValues(std::move(seedValues)) {
        mKeys.swap(mRealKeys);
        mValues.swap(mRealValues);
        mKeys.swap(mWindowKeys);
        mValues.swap(mWindowValues);
    }
    VectorWindow(VectorWindow const&)            = delete;
    VectorWindow& operator=(VectorWindow const&) = delete;
    ~VectorWindow() {
        mKeys.swap(mWindowKeys);
        mValues.swap(mWindowValues);
        mKeys.swap(mRealKeys);
        mValues.swap(mRealValues);
    }
};

enum class SentState : uint8 { Absent, Nullopt, Position };

template <class Position>
struct SentPosition {
    Position  mPosition{};
    SentState mState = SentState::Absent;
};

template <class Position>
class SentPositions {
    ll::SmallDenseMap<int64, size_t>     mSlots;
    std::vector<size_t>                  mFree;
    std::vector<SentPosition<Position>>  mEntries;
    size_t                               mCapacity{};
    size_t                               mUsed{};

    void grow(size_t capacity) {
        std::vector<SentPosition<Position>> entries(capacity * capacity);
        for (size_t i = 0; i < mCapacity; ++i) {
            std::copy_n(mEntries.begin() + i * mCapacity, mCapacity, entries.begin() + i * capacity);
        }
        mEntries = std::move(entries);
        mCapacity = capacity;
    }

public:
    void clear() {
        mSlots.clear();
        mFree.clear();
        mEntries.clear();
        mCapacity = 0;
        mUsed     = 0;
    }

    size_t slot(int64 id) {
        if (auto it = mSlots.find(id); it != mSlots.end()) return it->second;
        size_t slot;
        if (!mFree.empty()) {
            slot = mFree.back();
            mFree.pop_back();
        } else {
            slot = mUsed++;
            if (slot >= mCapacity) grow(mCapacity == 0 ? 128 : mCapacity * 2);
        }
        mSlots.emplace(id, slot);
        return slot;
    }

    SentPosition<Position>& at(size_t viewer, size_t observed) {
        return mEntries[viewer * mCapacity + observed];
    }

    void remove(int64 id) {
        auto it = mSlots.find(id);
        if (it == mSlots.end()) return;
        auto slot = it->second;
        for (size_t i = 0; i < mCapacity; ++i) {
            at(slot, i) = {};
            at(i, slot) = {};
        }
        mSlots.erase(it);
        mFree.push_back(slot);
    }
};

} // namespace lo::features::detail
