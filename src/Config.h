#pragma once

#include "features/features.h"
#include "ll/api/reflection/Dispatcher.h"

namespace lo {
using ll::reflection::Dispatcher;
struct Config {

    int version = 19;

    struct {
        Dispatcher<bool, moving_block_opt::MovingBlockOpt>                  optMovingBlock = false;
        Dispatcher<bool, chunk_leak_fix::ChunkLeakFix>                      fixChunkLeak   = true;
        Dispatcher<bool, seen_percent_opt::SeenPercentOpt>                  optSeenPercent = false;
        Dispatcher<push_entity_opt::Config, push_entity_opt::PushEntityOpt> optPushEntity{};
        Dispatcher<bool, player_networking_opt::PlayerNetworkingOpt>        optPlayerNetworking   = false;
        Dispatcher<bool, random_chunk_opt::RandomChunkOpt>                  optRandomChunk        = false;
        Dispatcher<bool, circuit_scene_graph_opt::CircuitSceneGraphOpt>     optCircuitSceneGraph  = false;
        Dispatcher<bool, chunk_gen_dispatcher_opt::ChunkGenDispatcherOpt>   optChunkGenDispatcher = false;
        Dispatcher<bool, chunk_caches_opt::ChunkCachesOpt>                  optChunkCaches        = false;
    } features{};
    struct {
        bool timingCommand = true;
    } commands{};
};

} // namespace lo
