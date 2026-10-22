// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.
// Licensed under the Apache License, Version 2.0; see LICENSES/Apache-2.0.txt.
// Adapted from Endstone commit 099d6f028: batch chunk influence updates.
// Modifications: nested calls and exception unwinding have scoped, thread-local state.

#include "features.h"
#include "ChunkOptimizationState.h"

#include "ll/api/memory/Hook.h"
#include "mc/world/level/PlayerTickManager.h"
#include "mc/world/level/dimension/ChunkBuildOrderPolicy.h"

namespace lo::player_networking_opt {

using InfluenceBatch = detail::InfluenceBatch<ChunkBuildOrderPolicy>;

LL_TYPE_INSTANCE_HOOK(
    PlayerNetworkingHook,
    ll::memory::HookPriority::Normal,
    PlayerTickManager,
    &PlayerTickManager::processPlayerNetworking,
    void,
    Tick const& currentTick
) {
    InfluenceBatch batch;
    origin(currentTick);
    batch.finish([](ChunkBuildOrderPolicy* policy) { policy->updateInfluences(); });
}

LL_TYPE_INSTANCE_HOOK(
    UpdateInfluencesHook,
    ll::memory::HookPriority::Normal,
    ChunkBuildOrderPolicy,
    &ChunkBuildOrderPolicy::$updateInfluences,
    void
) {
    // Calls outside player networking keep their original timing and behavior.
    if (!InfluenceBatch::defer(this)) origin();
}

struct PlayerNetworkingOpt::Impl {
    ll::memory::HookRegistrar<PlayerNetworkingHook, UpdateInfluencesHook> hooks;
};

void PlayerNetworkingOpt::call(bool enable) {
    if (enable) {
        if (!impl) impl = std::make_unique<Impl>();
    } else {
        impl.reset();
    }
}

PlayerNetworkingOpt::PlayerNetworkingOpt()                                          = default;
PlayerNetworkingOpt::~PlayerNetworkingOpt()                                         = default;
PlayerNetworkingOpt::PlayerNetworkingOpt(PlayerNetworkingOpt&&) noexcept            = default;
PlayerNetworkingOpt& PlayerNetworkingOpt::operator=(PlayerNetworkingOpt&&) noexcept = default;

} // namespace lo::player_networking_opt
