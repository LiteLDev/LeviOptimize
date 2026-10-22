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
#include <expected>
#include <span>
#include <string_view>
#include <vector>

#include <Windows.h>

namespace lo::detail {

enum class FunctionCodeError {
    SymbolNotFound,
    UnwindInfoUnavailable,
    ForeignImage,
    NotFunctionEntry,
    InvalidFunctionBounds,
};

[[nodiscard]] constexpr std::string_view errorMessage(FunctionCodeError error) noexcept {
    switch (error) {
    case FunctionCodeError::SymbolNotFound:
        return "BDS symbol was not found";
    case FunctionCodeError::UnwindInfoUnavailable:
        return "Windows unwind information is unavailable";
    case FunctionCodeError::ForeignImage:
        return "resolved function is outside the main executable";
    case FunctionCodeError::NotFunctionEntry:
        return "resolved address is not the exact function entry";
    case FunctionCodeError::InvalidFunctionBounds:
        return "Windows unwind information has invalid function bounds";
    }
    return "unknown function resolution error";
}

[[nodiscard]] inline std::expected<std::span<unsigned char>, FunctionCodeError>
getFunctionCode(std::string_view symbol) {
    // Runtime imports may still contain delay-load helpers while loading config.
    auto* function = ll::memory::SymbolView(symbol).resolve(true);
    if (!function) return std::unexpected(FunctionCodeError::SymbolNotFound);
    auto    address = reinterpret_cast<DWORD64>(function);
    DWORD64 imageBase{};
    auto*   entry = RtlLookupFunctionEntry(address, &imageBase, nullptr);
    if (!entry) return std::unexpected(FunctionCodeError::UnwindInfoUnavailable);
    if (imageBase != reinterpret_cast<DWORD64>(GetModuleHandleW(nullptr))) {
        return std::unexpected(FunctionCodeError::ForeignImage);
    }
    auto begin = imageBase + entry->BeginAddress;
    auto end   = imageBase + entry->EndAddress;
    // Never follow a hook/import thunk or expand to a process-wide scan.
    if (begin != address) return std::unexpected(FunctionCodeError::NotFunctionEntry);
    if (end <= begin) return std::unexpected(FunctionCodeError::InvalidFunctionBounds);
    return std::span<unsigned char>{reinterpret_cast<unsigned char*>(begin), static_cast<size_t>(end - begin)};
}

enum class BytePatchError {
    AlreadyInstalled,
    InvalidRequest,
    InstructionsChanged,
};

[[nodiscard]] constexpr std::string_view errorMessage(BytePatchError error) noexcept {
    switch (error) {
    case BytePatchError::AlreadyInstalled:
        return "patch is already installed";
    case BytePatchError::InvalidRequest:
        return "patch offset or replacement byte is invalid";
    case BytePatchError::InstructionsChanged:
        return "instruction bytes or signature changed during validation";
    }
    return "unknown byte patch error";
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
    [[nodiscard]] std::expected<void, BytePatchError> install(
        std::span<unsigned char> code,
        size_t                   offset,
        unsigned char            expected,
        unsigned char            value,
        Validate&&               validate
    ) {
        if (!mInstructions.empty()) return std::unexpected(BytePatchError::AlreadyInstalled);
        if (offset >= code.size() || expected == value) return std::unexpected(BytePatchError::InvalidRequest);
        if (code[offset] != expected || !validate(code)) return std::unexpected(BytePatchError::InstructionsChanged);
        mOriginal.assign(code.begin(), code.end());
        std::expected<void, BytePatchError> result = std::unexpected(BytePatchError::InstructionsChanged);
        // LeviLamina's modify pauses the other threads once the game is running,
        // makes this byte writable, then restores the protection and threads.
        // Its void API exposes no protection failure status. Preserve lower-level
        // exceptions: a failed operation cannot be assumed to leave BDS unchanged.
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
            result        = {};
            FlushInstructionCache(GetCurrentProcess(), code.data() + offset, 1);
        });
        return result;
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
