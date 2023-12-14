#pragma once

#include "features/features.h"
#include "ll/api/reflection/Dispatcher.h"


namespace lo {

struct Config {

    int version = 0;

    struct {
        ll::reflection::Dispatcher<bool, moving_block_fix::MovingBlockFix> fixMovingBlock = true;
        ll::reflection::Dispatcher<bool, hopper_item_fix::HopperItemFix>   fixHopperItem  = true;
    } features{};
};

extern Config globalConfig;

bool loadLoConfig();

bool saveLoConfig();

} // namespace lo
