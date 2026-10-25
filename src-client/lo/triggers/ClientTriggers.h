#pragma once
#include "lo/triggers/ITrigger.h"
#include <ll/api/event/ListenerBase.h>

namespace lo::triggers {

class ClientJoinedWorld final : public Trigger<ClientJoinedWorld> {
public:
    ClientJoinedWorld();
    ~ClientJoinedWorld() override;

private:
    ll::event::ListenerPtr mListener;
};

class ClientLeftWorld final : public Trigger<ClientLeftWorld> {
public:
    ClientLeftWorld();
    ~ClientLeftWorld() override;

private:
    ll::event::ListenerPtr mListener;
};

} // namespace lo::triggers
