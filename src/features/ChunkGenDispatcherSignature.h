// Adapted from Endstone commit 7049af40e7ec7364627d9a2a1e14ebe4c2eb714d.
// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Modifications: a bounded, unique signature lookup for the reversible patch.

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>

namespace lo::chunk_gen_dispatcher_opt::detail {

// Load the async pool, measure its worker vector, divide the byte size by eight,
// then form the 2n/6n in-flight task limits. A shift of one instead of three
// raises those limits to 8n/24n; only the RIP-relative displacement may vary.
inline constexpr std::array<unsigned char, 25> WorkerCountInstructions{0x48, 0x8B, 0x05, 0,    0,    0,    0,
                                                                       0x48, 0x8B, 0x78, 0x28, 0x48, 0x2B, 0x78,
                                                                       0x20, 0x48, 0xC1, 0xEF, 0x03, 0x01, 0xFF,
                                                                       0x44, 0x8D, 0x34, 0x7F};
inline constexpr std::size_t                   ShiftImmediate = 18;

[[nodiscard]] constexpr bool matchesWorkerCountInstructions(std::span<unsigned char const> code) {
    if (code.size() < WorkerCountInstructions.size()) return false;
    for (std::size_t i = 0; i < WorkerCountInstructions.size(); ++i) {
        if (i >= 3 && i < 7) continue;
        if (code[i] != WorkerCountInstructions[i]) return false;
    }
    return true;
}

// Missing, incomplete and ambiguous signatures all refuse the patch.
[[nodiscard]] constexpr std::optional<std::size_t> findWorkerCountInstructions(std::span<unsigned char const> code) {
    if (code.size() < WorkerCountInstructions.size()) return std::nullopt;
    std::optional<std::size_t> match;
    for (std::size_t i = 0; i <= code.size() - WorkerCountInstructions.size(); ++i) {
        if (!matchesWorkerCountInstructions(code.subspan(i, WorkerCountInstructions.size()))) continue;
        if (match) return std::nullopt;
        match = i;
    }
    return match;
}

} // namespace lo::chunk_gen_dispatcher_opt::detail
