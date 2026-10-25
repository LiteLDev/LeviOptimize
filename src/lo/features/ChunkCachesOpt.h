#pragma once
#include "lo/features/IFeature.h"
#include <ipatch/ipatch.hpp>
#include <optional>

namespace lo::features {

// Spread players overflow both chunk caches every few seconds and BDS then wipes
// them whole; raise the caps from 10,000 and 30,000 entries to about a million.
class ChunkCachesOpt final : public Feature<ChunkCachesOpt> {
public:
    using Config = bool;
    static constexpr FeatureSide side = FeatureSide::server;

    ChunkCachesOpt();
    ~ChunkCachesOpt() override;

private:
    std::optional<ipatch::Patch> mBlending;
    std::optional<ipatch::Patch> mExistence;

protected:
    ll::Expected<> onEnable() override;
    void           onDisable() override;
};

} // namespace lo::features
