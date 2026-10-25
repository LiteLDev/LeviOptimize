// Ported from Endstone bb0475b6:
// https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/src/endstone/runtime/hook.cpp#L172-L193
// Signatures: https://github.com/EndstoneMC/endstone/blob/bb0475b64744106b328238b3f51d77576ddc6569/scripts/configs/windows.toml#L22-L54

#include "lo/features/ChunkCachesOpt.h"
#include <ll/api/i18n/I18n.h>
#include <utility>

namespace lo::features {

using namespace fmt::literals;

ChunkCachesOpt::ChunkCachesOpt()  = default;
ChunkCachesOpt::~ChunkCachesOpt() = default;

ll::Expected<> ChunkCachesOpt::onEnable() {
    // Per-ChunkPos blender cache: cmp qword ptr [rsi+?], 0x2710; raise the
    // 10,000-entry cap to 0x102710 (about a million).
    auto blending = ipatch::Patch::search("48 81 7E ? 10 27 00 00 0F 82") //
                        .unique()
                        .offset(4)
                        .expect<uint32>(0x2710)
                        .write<uint32>(0x102710)
                        .apply();
    if (!blending) {
        return ll::makeI18nStringError<"chunk caches: chunk blending: {error}">("error"_a = blending.error().format());
    }

    // Chunk-existence cache: cmp qword ptr [r13+?], 0x7530; raise the
    // 30,000-entry cap to 0x107530 (about a million).
    auto existence = ipatch::Patch::search("49 81 BD ? ? ? ? 30 75 00 00 0F 82")
                         .unique()
                         .offset(7)
                         .expect<uint32>(0x7530)
                         .write<uint32>(0x107530)
                         .apply();
    if (!existence) {
        // The local blending Patch above restores itself on this early return.
        return ll::makeI18nStringError<"chunk caches: chunk existence: {error}">(
            "error"_a = existence.error().format()
        );
    }

    mBlending.emplace(std::move(*blending));
    mExistence.emplace(std::move(*existence));
    return {};
}

void ChunkCachesOpt::onDisable() {
    mBlending.reset();
    mExistence.reset();
}

LO_REGISTER_FEATURE(ChunkCachesOpt);

} // namespace lo::features
