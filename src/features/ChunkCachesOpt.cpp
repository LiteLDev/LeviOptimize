// Adapted from Endstone commit 2bf178a9972dd8cbe3fb3f81f10dd1077fafc4ce.
// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Modifications: resolve named LeviLamina symbols, reject ambiguous signatures,
// verify the complete sequence again under modify, and restore on disable.

#include "features.h"
#include "ChunkCacheSignatures.h"
#include "LeviOptimize.h"
#include "VerifiedBytePatch.h"

#include <string_view>

namespace lo::chunk_caches_opt {

namespace {
// These decorated names were confirmed with clang-cl from the MCAPI method
// declarations in LeviLamina 26.51.7, rather than copied from an address table.
constexpr std::string_view BlendingSymbol =
    "?getOrCreateChunkBlender@ChunkBlenderFactory@@QEAA?AV?$shared_ptr@VChunkBlender@@@std@@AEBVChunkPos@@@Z";
constexpr std::string_view ExistenceSymbol = "?_hasChunk@DBChunkStorage@@QEAA_NAEBVDBChunkStorageKey@@@Z";

template <size_t Size>
void installCachePatch(
    lo::detail::VerifiedBytePatch&      patch,
    std::string_view                    symbol,
    detail::CacheSignature<Size> const& signature,
    std::string_view                    name
) {
    auto code = lo::detail::getFunctionCode(symbol);
    if (code.size() < signature.bytes.size()) {
        getLogger().warn("optChunkCaches: unavailable {} function boundary; leaving its cache limit unchanged.", name);
        return;
    }
    auto offset = signature.find(code);
    if (!offset) {
        getLogger().warn(
            "optChunkCaches: unexpected or ambiguous {} instructions; leaving its cache limit unchanged.",
            name
        );
        return;
    }
    auto instructions = code.subspan(*offset, signature.bytes.size());
    if (!patch.install(instructions, signature.byteOffset, signature.expected, signature.replacement, [&](auto bytes) {
            return signature.matches(bytes);
        })) {
        getLogger().warn(
            "optChunkCaches: {} instructions changed during validation; leaving its cache limit unchanged.",
            name
        );
    }
}
} // namespace

struct ChunkCachesOpt::Impl {
    lo::detail::VerifiedBytePatch blending;
    lo::detail::VerifiedBytePatch existence;

    Impl() {
        installCachePatch(blending, BlendingSymbol, detail::BlendingCache, "chunk blending");
        installCachePatch(existence, ExistenceSymbol, detail::ExistenceCache, "chunk existence");
    }
};

void ChunkCachesOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

ChunkCachesOpt::ChunkCachesOpt()                                     = default;
ChunkCachesOpt::~ChunkCachesOpt()                                    = default;
ChunkCachesOpt::ChunkCachesOpt(ChunkCachesOpt&&) noexcept            = default;
ChunkCachesOpt& ChunkCachesOpt::operator=(ChunkCachesOpt&&) noexcept = default;

} // namespace lo::chunk_caches_opt
