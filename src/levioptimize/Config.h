#pragma once

#include "features/features.h"
#include "ll/api/reflection/Dispatcher.h"

namespace lo {
using ll::reflection::Dispatcher;
struct Config {

    int version = 13;

    struct {
        Dispatcher<bool, moving_block_opt::MovingBlockOpt>                           optMovingBlock   = true;
        Dispatcher<bool, hopper_item_opt::HopperItemOpt>                             optHopperItem    = true;
        Dispatcher<bool, chunk_leak_fix::ChunkLeakFix>                               fixChunkLeak     = true;
        Dispatcher<bool, power_association_map_leak_fix::PowerAssociationMapLeakFix> fixPowerAssocMap = true;
        Dispatcher<bool, timer_fix::TimerFix>                                        fixTimer         = true;
        Dispatcher<bool, seen_percent_opt::SeenPercentOpt>                           optSeenPercent   = true;
        Dispatcher<packet_sender_opt::Config, packet_sender_opt::PacketSenderOpt>    packetSenderOpt  = true;
        Dispatcher<bool, player_lookup_opt::PlayerLookupOpt>                         playerLookupOpt  = true;
        Dispatcher<push_entity_opt::Config, push_entity_opt::PushEntityOpt>          optPushEntity{};
    } features{};
    struct {
        bool timingCommand = true;
    } commands{};
};

} // namespace lo
