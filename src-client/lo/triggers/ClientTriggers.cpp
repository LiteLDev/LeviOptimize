#include "lo/triggers/ClientTriggers.h"
#include <ll/api/event/EventBus.h>
#include <ll/api/event/client/ClientExitLevelEvent.h>
#include <ll/api/event/client/ClientJoinLevelEvent.h>

namespace lo::triggers {

using namespace ll::event;

ClientJoinedWorld::ClientJoinedWorld() {
    mListener = EventBus::getInstance().emplaceListener<ClientJoinLevelEvent>([this](auto&) { apply(); });
}
ClientJoinedWorld::~ClientJoinedWorld() { EventBus::getInstance().removeListener(mListener); }

ClientLeftWorld::ClientLeftWorld() {
    mListener = EventBus::getInstance().emplaceListener<ClientExitLevelEvent>([this](auto&) { apply(); });
}
ClientLeftWorld::~ClientLeftWorld() { EventBus::getInstance().removeListener(mListener); }

LO_REGISTER_TRIGGER(ClientJoinedWorld);
LO_REGISTER_TRIGGER(ClientLeftWorld);

} // namespace lo::triggers
