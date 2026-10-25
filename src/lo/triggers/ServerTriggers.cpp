#include "lo/triggers/ServerTriggers.h"
#include <ll/api/event/EventBus.h>
#include <ll/api/event/server/ServerStartedEvent.h>
#include <ll/api/event/server/ServerStoppingEvent.h>
#include <ll/api/service/Bedrock.h>

namespace lo::triggers {

using namespace ll::event;

bool ServerStarted::active() const noexcept {
    return ll::service::getServerInstance() && ll::service::getLevel();
}

ServerStarted::ServerStarted() {
    mListener = EventBus::getInstance().emplaceListener<ServerStartedEvent>([this](auto&&) { apply(); });
}
ServerStarted::~ServerStarted() { EventBus::getInstance().removeListener(mListener); }

ServerStopping::ServerStopping() {
    mListener = EventBus::getInstance().emplaceListener<ServerStoppingEvent>([this](auto&&) { apply(); });
}
ServerStopping::~ServerStopping() { EventBus::getInstance().removeListener(mListener); }

LO_REGISTER_TRIGGER(ServerStarted);
LO_REGISTER_TRIGGER(ServerStopping);

} // namespace lo::triggers
