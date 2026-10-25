#pragma once
#include <cstddef>
#include <ranges>
#include <vector>

namespace lo::features::detail {

// Keep keys only: saving must still resolve each key against the live chunk map.
template <class Position>
class ChunkSnapshot {
    static constexpr size_t RefreshInterval = 400;

    std::vector<Position> mPositions{};
    size_t                mPicksLeft{};

public:
    template <class Map>
    [[nodiscard]] Position pick(Map const& map, size_t index) {
        if (mPicksLeft == 0 || mPositions.empty()) {
            auto replacement = map | std::views::keys | std::ranges::to<std::vector<Position>>();
            mPositions.swap(replacement);
            mPicksLeft = RefreshInterval;
        }
        --mPicksLeft;
        return mPositions[index * mPositions.size() / map.size()];
    }
};

} // namespace lo::features::detail
