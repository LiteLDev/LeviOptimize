// Adapted from Endstone commit 7049af40e7ec7364627d9a2a1e14ebe4c2eb714d.
// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Modifications: resolve the LeviLamina symbol, bound the signature to its unwind
// entry, verify the complete instruction sequence, and restore it on disable.

#include "features.h"
#include "ChunkGenDispatcherSignature.h"
#include "LeviOptimize.h"
#include "VerifiedBytePatch.h"

namespace lo::chunk_gen_dispatcher_opt {

namespace {
using detail::ShiftImmediate;
using detail::WorkerCountInstructions;
} // namespace

struct ChunkGenDispatcherOpt::Impl {
    lo::detail::VerifiedBytePatch patch;

    Impl() {
        auto resolved = lo::detail::getFunctionCode("?checkAndLaunchChunkGenerationTasks@ChunkSource@@QEAAX_N@Z");
        if (!resolved) {
            getLogger().warn(
                "optChunkGenDispatcher: {}; leaving task limits unchanged.",
                lo::detail::errorMessage(resolved.error())
            );
            return;
        }
        auto code = *resolved;
        if (code.size() < WorkerCountInstructions.size()) {
            getLogger().warn(
                "optChunkGenDispatcher: function is too short for its signature; leaving task limits unchanged."
            );
            return;
        }

        auto offset = detail::findWorkerCountInstructions(code);
        if (!offset) {
            getLogger().warn(
                "optChunkGenDispatcher: unexpected or ambiguous instruction sequence; leaving task limits unchanged."
            );
            return;
        }
        auto instructions = code.subspan(*offset, WorkerCountInstructions.size());
        auto result       = patch.install(instructions, ShiftImmediate, 3, 1, detail::matchesWorkerCountInstructions);
        if (!result) {
            getLogger().warn(
                "optChunkGenDispatcher: {}; leaving task limits unchanged.",
                lo::detail::errorMessage(result.error())
            );
        }
    }
};

void ChunkGenDispatcherOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

ChunkGenDispatcherOpt::ChunkGenDispatcherOpt()                                            = default;
ChunkGenDispatcherOpt::~ChunkGenDispatcherOpt()                                           = default;
ChunkGenDispatcherOpt::ChunkGenDispatcherOpt(ChunkGenDispatcherOpt&&) noexcept            = default;
ChunkGenDispatcherOpt& ChunkGenDispatcherOpt::operator=(ChunkGenDispatcherOpt&&) noexcept = default;

} // namespace lo::chunk_gen_dispatcher_opt
