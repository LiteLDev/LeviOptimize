#pragma once

#include "features/features.h"
#include "ll/api/reflection/Dispatcher.h"

namespace lo {
using ll::reflection::Dispatcher;
struct Config {

    int version = 18;

    struct {
        Dispatcher<bool, moving_block_opt::MovingBlockOpt>                  optMovingBlock = false;
        Dispatcher<bool, chunk_leak_fix::ChunkLeakFix>                      fixChunkLeak   = true;
        Dispatcher<bool, seen_percent_opt::SeenPercentOpt>                  optSeenPercent = false;
        Dispatcher<push_entity_opt::Config, push_entity_opt::PushEntityOpt> optPushEntity{};
    } features{};
    struct {
        bool timingCommand = true;
    } commands{};
};

} // namespace lo
