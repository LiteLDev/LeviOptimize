#pragma once
#include "lo/triggers/ITrigger.h"
#include <ll/api/event/ListenerBase.h>

namespace lo::triggers {

class ServerStarted final : public Trigger<ServerStarted> {
private:
    ll::event::ListenerPtr mListener;

public:
    ServerStarted();
    ~ServerStarted() override;

    [[nodiscard]] bool active() const noexcept override;
};

class ServerStopping final : public Trigger<ServerStopping> {
private:
    ll::event::ListenerPtr mListener;

public:
    ServerStopping();
    ~ServerStopping() override;
};

} // namespace lo::triggers
