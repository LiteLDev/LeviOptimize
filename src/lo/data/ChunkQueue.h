#pragma once
#include <mc/server/ChunkPositionAndDimension.h>
#include <mc/world/level/chunk/LevelChunk.h>
#include <functional>

inline bool operator==(ChunkPositionAndDimension const& a, ChunkPositionAndDimension const& b) {
    return a.mPos->x == b.mPos->x && a.mPos->z == b.mPos->z && a.mType->mValue == b.mType->mValue;
}

template <>
struct std::hash<ChunkPositionAndDimension> {
    size_t operator()(ChunkPositionAndDimension const& key) const {
        size_t seed{};
        auto        part  = std::hash<int>{};
        seed             ^= part(key.mPos->x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed             ^= part(key.mPos->z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed             ^= part(key.mType->mValue) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};