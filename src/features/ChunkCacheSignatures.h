// Adapted from Endstone commit 2bf178a9972dd8cbe3fb3f81f10dd1077fafc4ce.
// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Modifications: bound each unique signature to its named function and expose
// the production matcher for tests without requiring a running BDS process.

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>

namespace lo::chunk_caches_opt::detail {

template <size_t Size>
struct CacheSignature {
    std::array<std::optional<unsigned char>, Size> bytes;
    size_t                                         byteOffset;
    unsigned char                                  expected;
    unsigned char                                  replacement;

    [[nodiscard]] constexpr bool matches(std::span<unsigned char const> code) const {
        if (code.size() < Size || byteOffset >= Size || code[byteOffset] != expected) return false;
        for (size_t i = 0; i < Size; ++i) {
            if (bytes[i] && code[i] != *bytes[i]) return false;
        }
        return true;
    }

    [[nodiscard]] constexpr std::optional<size_t> find(std::span<unsigned char const> code) const {
        if (code.size() < Size) return std::nullopt;
        std::optional<size_t> match;
        for (size_t i = 0; i <= code.size() - Size; ++i) {
            if (!matches(code.subspan(i, Size))) continue;
            if (match) return std::nullopt;
            match = i;
        }
        return match;
    }
};

// Windows CMP size, imm32; only the member displacement varies. Raising the
// third immediate byte adds 0x100000 entries to each cache's original limit.
inline constexpr CacheSignature<10> BlendingCache{
    {0x48, 0x81, 0x7E, std::nullopt, 0x10, 0x27, 0x00, 0x00, 0x0F, 0x82},
    6,
    0x00,
    0x10
};
inline constexpr CacheSignature<13> ExistenceCache{
    {0x49, 0x81, 0xBD, std::nullopt, std::nullopt, std::nullopt, std::nullopt, 0x30, 0x75, 0x00, 0x00, 0x0F, 0x82},
    9,
    0x00,
    0x10
};

} // namespace lo::chunk_caches_opt::detail
