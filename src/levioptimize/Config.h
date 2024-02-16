#pragma once

#include "features/features.h"
#include "ll/api/reflection/Dispatcher.h"

namespace lo {

struct Config {

    int version = 2;

    struct {
        ll::reflection::Dispatcher<bool, moving_block_fix::MovingBlockFix> fixMovingBlock = true;
        ll::reflection::Dispatcher<bool, hopper_item_fix::HopperItemFix>   fixHopperItem  = true;
        ll::reflection::Dispatcher<bool, chunk_leak_fix::ChunkLeakFix>     fixChunkLeak   = true;
        ll::reflection::Dispatcher<bool, push_entity_opt::PushEntityOpt>   optPushEntity  = true;
        ll::reflection::Dispatcher<bool, block_lookup_opt::BlockLookupOpt> optBlockLookup = true;
    } features{};
};

} // namespace lo
