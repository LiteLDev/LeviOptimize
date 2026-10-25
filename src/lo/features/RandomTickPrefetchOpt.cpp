// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/bedrock_hooks/level_chunk.cpp#L34-L135

#include "lo/features/RandomTickPrefetchOpt.h"
#include <ipatch/ipatch.hpp>
#include <ll/api/i18n/I18n.h>
#include <mc/world/level/ILevel.h>
#include <mc/world/level/block/Block.h>
#include <mc/world/level/block/BlockType.h>
#include <mc/world/level/block/registry/BlockTypeRegistry.h>
#include <mc/world/level/chunk/LevelChunk.h>
#include <mc/world/level/chunk/SubChunk.h>
#include <mc/world/level/chunk/SubChunkStorage.h>
#include <mc/world/level/storage/GameRule.h>
#include <mc/world/level/storage/GameRules.h>
#include <xmmintrin.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdlib>
#include <functional>
#include <string_view>

namespace lo::features {

using namespace fmt::literals;

constexpr size_t MaxWarmedSamples = 256;

LL_TYPE_INSTANCE_HOOK(
    RandomTickPrefetchOpt::WarmHook,
    HookPriority::Normal,
    LevelChunk,
    &LevelChunk::tickImpl,
    void,
    BlockSource&          tickRegion,
    Tick const&           tick,
    std::function<void()> spawnerCallback
) {
    // tickImpl invokes the callback synchronously, so the detour's stack values
    // outlive it; the shared_ptr keeps the feature alive across the origin call.
    origin(tickRegion, tick, [this, self = get(), &spawnerCallback] {
        if (spawnerCallback) spawnerCallback();
        self->warmRandomTicks(*this);
    });
}

RandomTickPrefetchOpt::RandomTickPrefetchOpt()  = default;
RandomTickPrefetchOpt::~RandomTickPrefetchOpt() = default;

ll::Expected<> RandomTickPrefetchOpt::onEnable() {
    // The LCG state is LevelChunkConstants::RANDVALUE, a std::atomic<int> shared by
    // lightning and the random-tick loop. The step at the random-tick site is:
    //   add  eax, 3C6EF35Fh      ; 05 5F F3 6E 3C
    //   xchg [rip+seed], eax     ; 87 05 ?? ?? ?? ??
    //   mov  eax, [rip+seed]     ; 8B 05 ?? ?? ?? ??
    //   sar  eax, 2              ; C1 F8 02
    // The reload's RIP displacement resolves the seed address.
    auto seed = ipatch::scan("05 5F F3 6E 3C 87 05 ?? ?? ?? ?? 8B 05 ?? ?? ?? ?? C1 F8 02")
                    .in_main_module()
                    .unique()
                    .offset(11)
                    .rel32(2, 6)
                    .ptr<std::atomic<int>>();
    if (!seed) {
        return ll::makeI18nStringError<"random tick prefetch: the tick seed signature is missing">();
    }

    mSeed = *seed;
    if (!mHooks) mHooks.emplace();
    return {};
}

void RandomTickPrefetchOpt::onDisable() {
    mHooks.reset();
    mSeed = nullptr;
}

void RandomTickPrefetchOpt::warmRandomTicks(LevelChunk const& chunk) {
    auto        seed  = static_cast<uint32>(mSeed->load(std::memory_order_relaxed));
    auto const& rules = *chunk.mLevel.getGameRules().mGameRules;
    auto        index = static_cast<size_t>(GameRules::GameRulesIndex::RandomTickSpeed);
    if (index >= rules.size()) return;

    auto* speed = std::get_if<int>(&*rules[index].mValue);
    if (!speed || *speed <= 0) return;

    auto* registry = chunk.mLevel.getBlockTypeRegistry().get().get();
    if (!registry || !registry->mDirectAccessBlocks) return;

    auto* air = registry->mDirectAccessBlocks->mAirBlock.mDefaultState;
    if (!air) return;

    auto const& subChunks = *chunk.mSubChunks;
    int         top       = -1;
    for (auto i = static_cast<int>(subChunks.size()) - 1; i >= 0; --i) {
        auto* standard = subChunks[static_cast<size_t>(i)].mBlocksReadPtr[0];
        if (!standard) return;

        auto* extra = subChunks[static_cast<size_t>(i)].mBlocksReadPtr[1];
        if (!standard->isPaletteUniform(*air) || (extra && !extra->isPaletteUniform(*air))) {
            top = i;
            break;
        }
    }
    if (top < 0) return;

    auto count  = static_cast<uint32>(static_cast<float>(top + 1) * 2.5F);
    auto total  = std::min(count * static_cast<uint32>(*speed), uint32{MaxWarmedSamples});
    auto height = static_cast<uint32>((top + 1) * 16);

    std::array<SubChunk const*, MaxWarmedSamples>               records{};
    std::array<uint16, MaxWarmedSamples>                        indices{};
    std::array<SubChunkStorage<Block> const*, MaxWarmedSamples> storages{};
    std::array<Block const*, MaxWarmedSamples>                  blocks{};

    auto n = size_t{};
    for (uint32 i = 0; i < total; ++i) {
        seed = seed * 3 + 0x3C6EF35F;
        // long is 32-bit on Windows and 64-bit on Linux.
        auto value = std::abs(static_cast<long>(seed) >> 2);
        auto x     = value & 15;
        auto z     = (value >> 8) & 15;
        auto y     = static_cast<uint32>(static_cast<int>((value >> 16) & 0xFFFF) % static_cast<int>(height));
        if ((y >> 4) >= subChunks.size()) continue;

        records[n] = &subChunks[y >> 4];
        indices[n] = static_cast<uint16>((x << 8) | (z << 4) | (y & 15));
        _mm_prefetch(reinterpret_cast<char const*>(&records[n]->mBlocksReadPtr[0]), _MM_HINT_T0);
        ++n;
    }

    for (size_t k = 0; k < n; ++k) {
        storages[k] = records[k]->mBlocksReadPtr[0];
        if (storages[k]) _mm_prefetch(reinterpret_cast<char const*>(storages[k]), _MM_HINT_T0);
    }

    for (size_t k = 0; k < n; ++k) {
        blocks[k] = storages[k] ? &storages[k]->getElement(indices[k]) : nullptr;
    }

    for (size_t k = 0; k < n; ++k) {
        if (blocks[k]) {
            _mm_prefetch(reinterpret_cast<char const*>(&blocks[k]->getBlockType().mEventManager), _MM_HINT_T0);
        }
    }
}

LO_REGISTER_FEATURE(RandomTickPrefetchOpt);

} // namespace lo::features
