// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Adapted from Endstone's guarded byte patches; adds scoped restoration,
// exact function bounds, and validation inside LeviLamina's modify operation.

#pragma once

#include "ll/api/memory/Memory.h"
#include "ll/api/memory/Symbol.h"

#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#include <Windows.h>

namespace lo::detail {

inline std::span<unsigned char> getFunctionCode(std::string_view symbol) {
    // Runtime imports may still contain delay-load helpers while loading config.
    auto* function = ll::memory::SymbolView(symbol).resolve(true);
    if (!function) return {};
    auto    address = reinterpret_cast<DWORD64>(function);
    DWORD64 imageBase{};
    auto*   entry = RtlLookupFunctionEntry(address, &imageBase, nullptr);
    if (!entry || imageBase != reinterpret_cast<DWORD64>(GetModuleHandleW(nullptr))) return {};
    auto begin = imageBase + entry->BeginAddress;
    auto end   = imageBase + entry->EndAddress;
    // Never follow a hook/import thunk or expand to a process-wide scan.
    if (begin != address || end <= begin) return {};
    return {reinterpret_cast<unsigned char*>(begin), static_cast<size_t>(end - begin)};
}

class VerifiedBytePatch {
    std::span<unsigned char>   mInstructions;
    std::vector<unsigned char> mOriginal;
    size_t                     mByteOffset{};
    unsigned char              mReplacement{};

public:
    VerifiedBytePatch() = default;
    ~VerifiedBytePatch() { reset(); }

    VerifiedBytePatch(VerifiedBytePatch const&)            = delete;
    VerifiedBytePatch& operator=(VerifiedBytePatch const&) = delete;
    VerifiedBytePatch(VerifiedBytePatch&&)                 = delete;
    VerifiedBytePatch& operator=(VerifiedBytePatch&&)      = delete;

    template <class Validate>
    bool install(
        std::span<unsigned char> code,
        size_t                   offset,
        unsigned char            expected,
        unsigned char            value,
        Validate&&               validate
    ) {
        if (!mInstructions.empty() || offset >= code.size() || expected == value) return false;
        if (code[offset] != expected || !validate(code)) return false;
        mOriginal.assign(code.begin(), code.end());
        bool installed{};
        // LeviLamina's modify pauses the other threads once the game is running,
        // makes this byte writable, then restores the protection and threads.
        ll::memory::modify(code.data() + offset, 1, [&] {
            // Recheck both the masked signature and every captured byte after
            // pausing: another mod may have modified code after the initial scan.
            if (code[offset] != expected || !validate(code)
                || std::memcmp(code.data(), mOriginal.data(), mOriginal.size()) != 0) {
                return;
            }
            code[offset]  = value;
            mInstructions = code;
            mByteOffset   = offset;
            mReplacement  = value;
            installed     = true;
            FlushInstructionCache(GetCurrentProcess(), code.data() + offset, 1);
        });
        return installed;
    }

    void reset() {
        if (mInstructions.empty()) return;
        auto code     = mInstructions;
        mInstructions = {};
        ll::memory::modify(code.data() + mByteOffset, 1, [&] {
            // Restore only our own complete installed sequence, including bytes
            // that were wildcards in the signature. Preserve later mods' changes.
            for (size_t i = 0; i < mOriginal.size(); ++i) {
                auto expected = i == mByteOffset ? mReplacement : mOriginal[i];
                if (code[i] != expected) return;
            }
            code[mByteOffset] = mOriginal[mByteOffset];
            FlushInstructionCache(GetCurrentProcess(), code.data() + mByteOffset, 1);
        });
    }
};

} // namespace lo::detail
