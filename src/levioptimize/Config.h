#pragma once

#include "features/features.h"
#include "ll/api/reflection/Dispatcher.h"

namespace lo {
using ll::reflection::Dispatcher;
struct Config {

    int version = 4;

    struct {
        Dispatcher<bool, moving_block_fix::MovingBlockFix> fixMovingBlock = true;
        Dispatcher<bool, hopper_item_fix::HopperItemFix>   fixHopperItem  = true;
        Dispatcher<bool, chunk_leak_fix::ChunkLeakFix>     fixChunkLeak   = true;
        Dispatcher<bool, push_entity_opt::PushEntityOpt>   optPushEntity  = true;
        Dispatcher<bool, block_lookup_opt::BlockLookupOpt> optBlockLookup = true;
        Dispatcher<bool, seen_percent_opt::SeenPercentOpt> optSeenPercent = true;
    } features{};
    struct {
        bool timingCommand = true;
    } commands{};
};

} // namespace lo
