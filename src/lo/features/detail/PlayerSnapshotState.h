#pragma once
#include <cstddef>
#include <cstring>
#include <span>
#include <vector>

namespace lo::features::detail {

class SnapshotState {
    std::vector<std::byte> mBytes;
    size_t                 mDepth{};
    bool                   mRetired{};

public:
    [[nodiscard]] bool matches(std::span<std::byte const> bytes) const noexcept {
        return !mRetired && mBytes.size() == bytes.size()
            && (bytes.empty() || std::memcmp(mBytes.data(), bytes.data(), bytes.size()) == 0);
    }
    [[nodiscard]] bool canRebuild() const noexcept { return !mRetired && mDepth == 0; }
    void               replace(std::span<std::byte const> bytes) { mBytes.assign(bytes.begin(), bytes.end()); }
    void               retire() noexcept { mRetired = true; }
    [[nodiscard]] bool retired() const noexcept { return mRetired; }

    class Visit {
        SnapshotState& mState;

    public:
        explicit Visit(SnapshotState& state) noexcept : mState(state) { ++mState.mDepth; }
        ~Visit() { --mState.mDepth; }
        Visit(Visit const&)            = delete;
        Visit& operator=(Visit const&) = delete;
    };
};

} // namespace lo::features::detail
